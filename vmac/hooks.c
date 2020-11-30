/*
* Copyright (c) 2017 - 2020, Mohammed Elbadry
*
*
* This file is part of V-MAC (Pub/Sub data-centric Multicast MAC layer)
*
* V-MAC is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 
* 4.0 International License.
* 
* You should have received a copy of the license along with this
* work. If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.
* 
*/
#include "vmac.h"
/**
 * 
 * These are all just hooks placed to connect V-MAC with hardware for ath9k_htc.
 */
const void *const mac80211_wiphy_privid = &mac80211_wiphy_privid;
struct ieee80211_hw* vhw;
struct ieee80211_vif* vmon;
void ieee80211_rx_ba_timer_expired(struct ieee80211_vif *vif, const u8 *addr, unsigned int tid)
{

}
EXPORT_SYMBOL(ieee80211_rx_ba_timer_expired);

void ieee80211_free_hw(struct ieee80211_hw *hw)
{
    /* TODO */
}
EXPORT_SYMBOL(ieee80211_free_hw);

int ieee80211_start_tx_ba_session(struct ieee80211_sta *pubsta, u16 tid, u16 timeout)
{
    /* TODO */
    return 0;
}
EXPORT_SYMBOL(ieee80211_start_tx_ba_session);


struct ieee80211_hw* getvhw(void)
{
    return vhw;
}

struct ieee80211_vif* getvmon(void)
{
    return vmon;
}

int ieee80211_register_hw(struct ieee80211_hw *hw)
{
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
    struct cfg80211_bitrate_mask wohomask= {};
    struct ieee80211_local *local = hw_to_local(hw);
    struct ieee80211_supported_band *sband;
    struct cfg80211_chan_def dflt_chandef = {};
    struct ieee80211_vif vif;
    struct ieee80211_tx_queue_params p;
    u32 changed,newflags;
    int result, i, j;
    struct ieee80211_bss_conf bss_conf;
    enum nl80211_band band;
    int channels, max_bitrates, ret;
    bool supp_ht, supp_vht;
    struct ieee80211_sta sta={};
    netdev_features_t feature_whitelist;
    vhw=hw;
    wohomask.control[NL80211_BAND_2GHZ].legacy=(u32)-1;
    
    if (ieee80211_hw_check(hw, QUEUE_CONTROL) &&
        (local->hw.offchannel_tx_hw_queue == IEEE80211_INVAL_HW_QUEUE ||
            local->hw.offchannel_tx_hw_queue >= local->hw.queues))
        return -EINVAL;

    if ((hw->wiphy->features & NL80211_FEATURE_TDLS_CHANNEL_SWITCH) &&
        (!local->ops->tdls_channel_switch ||
            !local->ops->tdls_cancel_channel_switch ||
            !local->ops->tdls_recv_channel_switch))
        return -EOPNOTSUPP;

    if (WARN_ON(ieee80211_hw_check(hw, SUPPORTS_TX_FRAG) &&
        !local->ops->set_frag_threshold))
        return -EINVAL;

    if (WARN_ON(local->hw.wiphy->interface_modes &
        BIT(NL80211_IFTYPE_NAN) &&
        (!local->ops->start_nan || !local->ops->stop_nan)))
        return -EINVAL;

    if (!local->use_chanctx) {
        for (i = 0; i < local->hw.wiphy->n_iface_combinations; i++) {
            const struct ieee80211_iface_combination *comb;

            comb = &local->hw.wiphy->iface_combinations[i];

            if (comb->num_different_channels > 1)
                return -EINVAL;
        }
    } else {
        /*
         * WDS is currently prohibited when channel contexts are used
         * because there's no clear definition of which channel WDS
         * type interfaces use
         */
        if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_WDS))
            return -EINVAL;

        /* DFS is not supported with multi-channel combinations yet */
        for (i = 0; i < local->hw.wiphy->n_iface_combinations; i++) {
            const struct ieee80211_iface_combination *comb;

            comb = &local->hw.wiphy->iface_combinations[i];

            if (comb->radar_detect_widths &&
                comb->num_different_channels > 1)
                return -EINVAL;
        }
    }

    /* Only HW csum features are currently compatible with V-MAC */
    feature_whitelist = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
    NETIF_F_HW_CSUM | NETIF_F_SG | NETIF_F_HIGHDMA |
    NETIF_F_GSO_SOFTWARE | NETIF_F_RXCSUM;
    if (WARN_ON(hw->netdev_features & ~feature_whitelist))
        return -EINVAL;

    if (hw->max_report_rates == 0)
        hw->max_report_rates = hw->max_rates;

    local->rx_chains = 1;

    /*
     * generic code guarantees at least one band,
     * set this very early because much code assumes
     * that hw.conf.channel is assigned
     */
    channels = 0;
    max_bitrates = 0;
    supp_ht = false;
    supp_vht = false;
    for (band = 0; band < NUM_NL80211_BANDS; band++) {
        

        sband = local->hw.wiphy->bands[band];
        if (!sband)
            continue;

        if (!dflt_chandef.chan) {
            cfg80211_chandef_create(&dflt_chandef,
                &sband->channels[0],
                NL80211_CHAN_WIDTH_40);
            /* init channel we're on */
            if (!local->use_chanctx && !local->_oper_chandef.chan) {
                local->hw.conf.chandef = dflt_chandef;
                local->_oper_chandef = dflt_chandef;
            }
            local->monitor_chandef = dflt_chandef;
        }

        channels += sband->n_channels;

        if (max_bitrates < sband->n_bitrates)
            max_bitrates = sband->n_bitrates;
        supp_ht = supp_ht || sband->ht_cap.ht_supported;
        supp_vht = supp_vht || sband->vht_cap.vht_supported;

        if (!sband->ht_cap.ht_supported)
            continue;

        /* TODO: consider VHT for RX chains, hopefully it's the same */
        //local->rx_chains =            max(ieee80211_mcs_to_chains(&sband->ht_cap.mcs),                local->rx_chains);

        /* no need to mask, SM_PS_DISABLED has all bits set */
        sband->ht_cap.cap |= WLAN_HT_CAP_SM_PS_DISABLED <<IEEE80211_HT_CAP_SM_PS_SHIFT;
        
    }

    /* if low-level driver supports AP, we also support VLAN */
    if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_AP)) {
        hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_AP_VLAN);
        hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_AP_VLAN);
    }

    /* always supports monitor */
    hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_MONITOR);
    hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);

    /* ce right now */
    for (i = 0; i < hw->wiphy->n_iface_combinations; i++) {
        const struct ieee80211_iface_combination *c;
        int j;

        c = &hw->wiphy->iface_combinations[i];

        for (j = 0; j < c->n_limits; j++)
            if ((c->limits[j].types & BIT(NL80211_IFTYPE_ADHOC)) &&
                c->limits[j].max > 1)
                return -EINVAL;
        }

        local->int_scan_req = kzalloc(sizeof(*local->int_scan_req) +
            sizeof(void *) * channels, GFP_KERNEL);
        if (!local->int_scan_req)
            return -ENOMEM;

        for (band = 0; band < NUM_NL80211_BANDS; band++) {
            if (!local->hw.wiphy->bands[band])
                continue;
            local->int_scan_req->rates[band] = (u32) -1;
        //sta->supp_rates[band]=0;
            sta.supp_rates[band]|=(u32)-1;
        }
        
#ifndef CONFIG_MAC80211_MESH
    /* mesh depends on Kconfig, but drivers should set it if they want */
        local->hw.wiphy->interface_modes &= ~BIT(NL80211_IFTYPE_MESH_POINT);
#endif

    /* if the underlying driver supports mesh, mac80211 will (at least)
     * provide routing of mesh authentication frames to userspace */
        if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_MESH_POINT))
            local->hw.wiphy->flags |= WIPHY_FLAG_MESH_AUTH;

    /* mac80211 supports control port protocol changing */
        local->hw.wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;

    /*
     * Calculate scan IE length -- we need this to alloc
     * memory and to subtract from the driver limit. It
     * includes the DS Params, (extended) supported rates, and HT
     * information -- SSID is the driver's responsibility.
     */
    local->scan_ies_len = 4 + max_bitrates /* (ext) supp rates */ + 3 /* DS Params */;
        if (supp_ht)
            local->scan_ies_len += 2 + sizeof(struct ieee80211_ht_cap);

        if (supp_vht)
            local->scan_ies_len +=          2 + sizeof(struct ieee80211_vht_cap);

        if (!local->ops->hw_scan) {
        /* For hw_scan, driver needs to set these up. */
            local->hw.wiphy->max_scan_ssids = 4;
            local->hw.wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;
        }

    /*
     * If the driver supports any scan IEs, then assume the
     * limit includes the IEs mac80211 will add, otherwise
     * leave it at zero and let the driver sort it out; we
     * still pass our IEs to the driver but userspace will
     * not be allowed to in that case.
     */
        if (local->hw.wiphy->max_scan_ie_len)
            local->hw.wiphy->max_scan_ie_len -= local->scan_ies_len;

        if (!local->ops->remain_on_channel)
            local->hw.wiphy->max_remain_on_channel_duration = 5000;

    /* mac80211 based drivers don't support internal TDLS setup */
        if (local->hw.wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS)
            local->hw.wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;

    /* mac80211 supports eCSA, if the driver supports STA CSA at all */
        if (ieee80211_hw_check(&local->hw, CHANCTX_STA_CSA))
            local->ext_capa[0] |= WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING;

        local->hw.wiphy->max_num_csa_counters = IEEE80211_MAX_CSA_COUNTERS_NUM;

        result = wiphy_register(local->hw.wiphy);
        
    /*
     * We use the number of queues for feature tests (QoS, HT) internally
     * so restrict them appropriately.
     */
        if (hw->queues > IEEE80211_MAX_QUEUES)
            hw->queues = IEEE80211_MAX_QUEUES;
    /*
     * The hardware needs headroom for sending the frame,
     * and we need some headroom for passing the frame to monitor
     * interfaces, but never both at the same time.
     */
        local->tx_headroom = max_t(unsigned int , local->hw.extra_tx_headroom,   IEEE80211_TX_STATUS_HEADROOM);

    /*
     * if the driver doesn't specify a max listen interval we
     * use 5 which should be a safe default
     */
        if (local->hw.max_listen_interval == 0)
        {
            local->hw.max_listen_interval = 5;
        }

        if (!local->hw.max_nan_de_entries)
            local->hw.max_nan_de_entries = IEEE80211_MAX_NAN_INSTANCE_ID;

    local->hw.conf.flags = IEEE80211_CONF_IDLE; //CHANGED LATER ON.......
    #else 
        struct cfg80211_bitrate_mask wohomask= {};
    struct ieee80211_local *local = hw_to_local(hw);
    struct ieee80211_supported_band *sband;
    struct cfg80211_chan_def dflt_chandef = {};
    struct ieee80211_vif vif;
    struct ieee80211_tx_queue_params p;
    u32 changed,newflags;
    int result, i, j;
    struct ieee80211_bss_conf bss_conf;
    enum nl80211_band band;
    int channels, max_bitrates, ret;
    bool supp_ht, supp_vht;
    struct ieee80211_sta sta={};
    netdev_features_t feature_whitelist;
    vhw=hw;
    wohomask.control[NL80211_BAND_2GHZ].legacy=(u32)-1;    

    if (ieee80211_hw_check(hw, QUEUE_CONTROL) &&
        (local->hw.offchannel_tx_hw_queue == IEEE80211_INVAL_HW_QUEUE ||
         local->hw.offchannel_tx_hw_queue >= local->hw.queues))
        return -EINVAL;

    if ((hw->wiphy->features & NL80211_FEATURE_TDLS_CHANNEL_SWITCH) &&
        (!local->ops->tdls_channel_switch ||
         !local->ops->tdls_cancel_channel_switch ||
         !local->ops->tdls_recv_channel_switch))
        return -EOPNOTSUPP;

    if (WARN_ON(local->hw.wiphy->interface_modes &
            BIT(NL80211_IFTYPE_NAN) &&
            (!local->ops->start_nan || !local->ops->stop_nan)))
        return -EINVAL;

#ifdef CONFIG_PM
    if (hw->wiphy->wowlan && (!local->ops->suspend || !local->ops->resume))
        return -EINVAL;
#endif

    if (!local->use_chanctx) {
        for (i = 0; i < local->hw.wiphy->n_iface_combinations; i++) {
            const struct ieee80211_iface_combination *comb;

            comb = &local->hw.wiphy->iface_combinations[i];

            if (comb->num_different_channels > 1)
                return -EINVAL;
        }
    } else {
        /*
         * WDS is currently prohibited when channel contexts are used
         * because there's no clear definition of which channel WDS
         * type interfaces use
         */
        if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_WDS))
            return -EINVAL;

        /* DFS is not supported with multi-channel combinations yet */
        for (i = 0; i < local->hw.wiphy->n_iface_combinations; i++) {
            const struct ieee80211_iface_combination *comb;

            comb = &local->hw.wiphy->iface_combinations[i];

            if (comb->radar_detect_widths &&
                comb->num_different_channels > 1)
                return -EINVAL;
        }
    }

    /* Only HW csum features are currently compatible with mac80211 */
    feature_whitelist = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
                NETIF_F_HW_CSUM | NETIF_F_SG | NETIF_F_HIGHDMA |
                NETIF_F_GSO_SOFTWARE | NETIF_F_RXCSUM;
    if (WARN_ON(hw->netdev_features & ~feature_whitelist))
        return -EINVAL;

    if (hw->max_report_rates == 0)
        hw->max_report_rates = hw->max_rates;

    local->rx_chains = 1;

    /*
     * generic code guarantees at least one band,
     * set this very early because much code assumes
     * that hw.conf.channel is assigned
     */
    channels = 0;
    max_bitrates = 0;
    supp_ht = false;
    supp_vht = false;
    for (band = 0; band < NUM_NL80211_BANDS; band++) {
        struct ieee80211_supported_band *sband;

        sband = local->hw.wiphy->bands[band];
        if (!sband)
            continue;

        if (!dflt_chandef.chan) {
            cfg80211_chandef_create(&dflt_chandef,
                        &sband->channels[0],
                        NL80211_CHAN_NO_HT);
            /* init channel we're on */
            if (!local->use_chanctx && !local->_oper_chandef.chan) {
                local->hw.conf.chandef = dflt_chandef;
                local->_oper_chandef = dflt_chandef;
            }
            local->monitor_chandef = dflt_chandef;
        }

        channels += sband->n_channels;

        if (max_bitrates < sband->n_bitrates)
            max_bitrates = sband->n_bitrates;
        supp_ht = supp_ht || sband->ht_cap.ht_supported;
        supp_vht = supp_vht || sband->vht_cap.vht_supported;

        if (!sband->ht_cap.ht_supported)
            continue;


        /* no need to mask, SM_PS_DISABLED has all bits set */
        sband->ht_cap.cap |= WLAN_HT_CAP_SM_PS_DISABLED <<
                         IEEE80211_HT_CAP_SM_PS_SHIFT;
    }

    /* if low-level driver supports AP, we also support VLAN */
    if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_AP)) {
        hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_AP_VLAN);
        hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_AP_VLAN);
    }

    /* mac80211 always supports monitor */
    hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_MONITOR);
    hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);

    /* mac80211 doesn't support more than one IBSS interface right now */
    for (i = 0; i < hw->wiphy->n_iface_combinations; i++) {
        const struct ieee80211_iface_combination *c;
        int j;

        c = &hw->wiphy->iface_combinations[i];

        for (j = 0; j < c->n_limits; j++)
            if ((c->limits[j].types & BIT(NL80211_IFTYPE_ADHOC)) &&
                c->limits[j].max > 1)
                return -EINVAL;
    }

    local->int_scan_req = kzalloc(sizeof(*local->int_scan_req) +
                      sizeof(void *) * channels, GFP_KERNEL);
    if (!local->int_scan_req)
        return -ENOMEM;

    for (band = 0; band < NUM_NL80211_BANDS; band++) {
        if (!local->hw.wiphy->bands[band])
            continue;
        local->int_scan_req->rates[band] = (u32) -1;
    }

#ifndef CONFIG_MAC80211_MESH
    /* mesh depends on Kconfig, but drivers should set it if they want */
    local->hw.wiphy->interface_modes &= ~BIT(NL80211_IFTYPE_MESH_POINT);
#endif

    /* if the underlying driver supports mesh, mac80211 will (at least)
     * provide routing of mesh authentication frames to userspace */
    if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_MESH_POINT))
        local->hw.wiphy->flags |= WIPHY_FLAG_MESH_AUTH;

    /* mac80211 supports control port protocol changing */
    local->hw.wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;

    if (ieee80211_hw_check(&local->hw, SIGNAL_DBM)) {
        local->hw.wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
    } else if (ieee80211_hw_check(&local->hw, SIGNAL_UNSPEC)) {
        local->hw.wiphy->signal_type = CFG80211_SIGNAL_TYPE_UNSPEC;
        if (hw->max_signal <= 0) {
            result = -EINVAL;
            
        }
    }

    /*
     * Calculate scan IE length -- we need this to alloc
     * memory and to subtract from the driver limit. It
     * includes the DS Params, (extended) supported rates, and HT
     * information -- SSID is the driver's responsibility.
     */
    local->scan_ies_len = 4 + max_bitrates /* (ext) supp rates */ +
        3 /* DS Params */;
    if (supp_ht)
        local->scan_ies_len += 2 + sizeof(struct ieee80211_ht_cap);

    if (supp_vht)
        local->scan_ies_len +=
            2 + sizeof(struct ieee80211_vht_cap);

    if (!local->ops->hw_scan) {
        /* For hw_scan, driver needs to set these up. */
        local->hw.wiphy->max_scan_ssids = 4;
        local->hw.wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;
    }

    /*
     * If the driver supports any scan IEs, then assume the
     * limit includes the IEs mac80211 will add, otherwise
     * leave it at zero and let the driver sort it out; we
     * still pass our IEs to the driver but userspace will
     * not be allowed to in that case.
     */
    if (local->hw.wiphy->max_scan_ie_len)
        local->hw.wiphy->max_scan_ie_len -= local->scan_ies_len;

    

    
    if (result < 0)
        

    if (!local->ops->remain_on_channel)
        local->hw.wiphy->max_remain_on_channel_duration = 5000;

    /* mac80211 based drivers don't support internal TDLS setup */
    if (local->hw.wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS)
        local->hw.wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;

    /* mac80211 supports eCSA, if the driver supports STA CSA at all */
    if (ieee80211_hw_check(&local->hw, CHANCTX_STA_CSA))
        local->ext_capa[0] |= WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING;

    local->hw.wiphy->max_num_csa_counters = IEEE80211_MAX_CSA_COUNTERS_NUM;

    result = wiphy_register(local->hw.wiphy);
    if (result < 0)
        

    /*
     * We use the number of queues for feature tests (QoS, HT) internally
     * so restrict them appropriately.
     */
    if (hw->queues > IEEE80211_MAX_QUEUES)
        hw->queues = IEEE80211_MAX_QUEUES;

    local->workqueue =
        alloc_ordered_workqueue("%s", 0, wiphy_name(local->hw.wiphy));
    if (!local->workqueue) {
        result = -ENOMEM;
        
    }

    /*
     * The hardware needs headroom for sending the frame,
     * and we need some headroom for passing the frame to monitor
     * interfaces, but never both at the same time.
     */
    local->tx_headroom = max_t(unsigned int , local->hw.extra_tx_headroom,
                   IEEE80211_TX_STATUS_HEADROOM);

    

    /*
     * if the driver doesn't specify a max listen interval we
     * use 5 which should be a safe default
     */
    if (local->hw.max_listen_interval == 0)
        local->hw.max_listen_interval = 5;

    local->hw.conf.listen_interval = local->hw.max_listen_interval;

    local->dynamic_ps_forced_timeout = -1;

    if (!local->hw.max_nan_de_entries)
        local->hw.max_nan_de_entries = IEEE80211_MAX_NAN_INSTANCE_ID;

    

    local->hw.conf.flags = IEEE80211_CONF_IDLE;

    


    local->hw.conf.flags = IEEE80211_CONF_IDLE; //CHANGED LATER ON.......
#endif
    //local->hw.rate_control_algorithm=kzalloc(3,GFP_ATOMIC);
    //local->hw.rate_control_algorithm="oka";
    //ieee80211_led_init(local); not needed.....
    local->started = true;
    dflt_chandef.chan=kzalloc(sizeof(struct ieee80211_channel),GFP_KERNEL);
    dflt_chandef.chan->max_power=20;
    dflt_chandef.chan->max_reg_power=20;
    local->hw.conf.chandef.chan=kzalloc(sizeof(struct ieee80211_channel),GFP_KERNEL);
    local->hw.conf.chandef.chan->hw_value=0;
    local->hw.conf.chandef.chan->flags=384;
    local->hw.conf.chandef.chan->center_freq=2447;
    local->hw.conf.chandef.center_freq1=2457;
    local->hw.conf.chandef.width = NL80211_CHAN_WIDTH_40;

    local->hw.conf.chandef.chan->max_power=20;
    local->hw.conf.chandef.chan->max_antenna_gain=1;
    local->hw.conf.chandef.chan->orig_mpwr=20;
    local->hw.conf.chandef.chan->orig_mag=20;
    ret= local->ops->start(&local->hw);
    local->hw.conf.power_level=20;
    bss_conf.enable_beacon=0;
    memset(&p, 0, sizeof(p));
    p.aifs = 0;
    p.cw_max =7;
    p.cw_min = 1;
    //p.txop = 2000;//disable burst for now...
    vif.type=NL80211_IFTYPE_ADHOC;
    vif.addr[0]=0xfe;
    vif.addr[1]=0xfe;
    vif.addr[2]=0xfe;
    vif.addr[3]=0xfe;
    vif.addr[4]=0xfe;
    vif.addr[5]=0xfe;
    sta.addr[0]=0xff;
    sta.addr[1]=0xff;
    sta.addr[2]=0xff;
    sta.addr[3]=0xff;
    sta.addr[4]=0xff;
    sta.addr[5]=0xff;
    vmon=&vif;
    changed=IEEE80211_CONF_CHANGE_POWER;
    sta.ht_cap.cap = IEEE80211_HT_CAP_RX_STBC| IEEE80211_HT_CAP_SUP_WIDTH_20_40| IEEE80211_HT_CAP_SGI_40| IEEE80211_HT_CAP_SGI_20;
    sta.ht_cap.ht_supported = true;
        for (i = 0, j = 0; i < 77; i++) {
            sta.ht_cap.mcs.rx_mask[i/8] |= (1<<(i%8));
        }
    //sta->ht_cap.ht_supported
    //sta->ht_cap.cap & IEEE80211_HT_CAP_RX_STBC
//  stmon=&sta;
    vif.bss_conf.basic_rates=(u32)-1;
    vif.bss_conf.txpower=20;
    //vif.bss_conf.use_short_preamble=;
    //vif.bss_conf.bssid=&bssid[0];
    local->ops->add_interface(&local->hw,vmon);
    local->ops->sta_add(&local->hw,vmon,&sta);
    vif.bss_conf.txpower=20;
    changed=BSS_CHANGED_HT;    
    local->ops->bss_info_changed(&local->hw,vmon,&bss_conf,changed);
    local->ops->conf_tx(&local->hw, vmon,0, &p);
    local->ops->conf_tx(&local->hw, vmon,1, &p);
    local->ops->conf_tx(&local->hw, vmon,2, &p);
    p.aifs=0;
    changed=FIF_OTHER_BSS;
    newflags=changed;
    local->ops->configure_filter(&local->hw,changed,&newflags,0);
    changed = IEEE80211_CONF_CHANGE_POWER;
    //local->hw.conf.flags&=~IEEE80211_CONF_PS;
    //changed |= IEEE80211_CONF_CHANGE_PS;
    //changed|=BSS_CHANGED_BEACON_ENABLED;
    //local->ops->config(&local->hw,changed);//must be done after adding interface to prevent hardware from adding another one and being dumb as usual
    return 0;
}
EXPORT_SYMBOL(ieee80211_register_hw);

const char *__ieee80211_create_tpt_led_trigger(struct ieee80211_hw *hw,
    unsigned int flags,
    const struct ieee80211_tpt_blink *blink_table,
    unsigned int blink_table_len)
{
    /* TODO */
    return "Wlan1";
}
EXPORT_SYMBOL(__ieee80211_create_tpt_led_trigger);

const char *__ieee80211_get_radio_led_name(struct ieee80211_hw *hw)
{
    /* TODO */
    return "Wlan1";
}
EXPORT_SYMBOL(__ieee80211_get_radio_led_name);

struct ieee80211_sta *ieee80211_find_sta_by_ifaddr(struct ieee80211_hw *hw,
    const u8 *addr,
    const u8 *localaddr)
{   
    return NULL;
}
EXPORT_SYMBOL_GPL(ieee80211_find_sta_by_ifaddr);

struct ieee80211_sta *ieee80211_find_sta(struct ieee80211_vif *vif,
    const u8 *addr)
{
    return NULL;
}
EXPORT_SYMBOL(ieee80211_find_sta);

void ieee80211_stop_tx_ba_cb_irqsafe(struct ieee80211_vif *vif,
    const u8 *ra, u16 tid)
{

}
EXPORT_SYMBOL(ieee80211_stop_tx_ba_cb_irqsafe);

struct ieee80211_hw *wiphy_to_ieee80211_hw(struct wiphy *wiphy)
{
    struct ieee80211_local *local;
    BUG_ON(!wiphy);

    local = wiphy_priv(wiphy);
    return &local->hw;
}
EXPORT_SYMBOL(wiphy_to_ieee80211_hw);

void ieee80211_queue_delayed_work(struct ieee80211_hw *hw,
    struct delayed_work *dwork,
    unsigned long delay)
{
    /* TODO */
}
EXPORT_SYMBOL(ieee80211_queue_delayed_work);

void ieee80211_wake_queues(struct ieee80211_hw *hw)
{

}
EXPORT_SYMBOL(ieee80211_wake_queues);

/**
 * @brief      hardware allocation function from ath9k_htc NOTE NEEDS CLEANING *FIXME*
 *
 * @param[in]  priv_data_len   The priv data length
 * @param[in]  ops             The ops
 * @param[in]  requested_name  The requested name
 *
 * @return     { description_of_the_return_value }
 */
struct ieee80211_hw *ieee80211_alloc_hw_nm(size_t priv_data_len,
    const struct ieee80211_ops *ops,
    const char *requested_name)
{
    struct ieee80211_local *local;
    int priv_size, i;
    struct wiphy *wiphy;
    bool use_chanctx;
#if LINUX_VERSION_CODE  >= KERNEL_VERSION(4,10,0)
    if (WARN_ON(!ops->tx || !ops->start || !ops->stop || !ops->config ||
        !ops->add_interface || !ops->remove_interface ||
        !ops->configure_filter))
        return NULL;

    if (WARN_ON(ops->sta_state && (ops->sta_add || ops->sta_remove)))
        return NULL;
    /* check all or no channel context operations exist */
    i = !!ops->add_chanctx + !!ops->remove_chanctx +
    !!ops->change_chanctx + !!ops->assign_vif_chanctx +
    !!ops->unassign_vif_chanctx;
    if (WARN_ON(i != 0 && i != 5))
        return NULL;
    use_chanctx = i == 5;
    /* Ensure 32-byte alignment of our private data and hw private data.
     * We use the wiphy priv data for both our ieee80211_local and for
     * the driver's private data
     *
     * In memory it'll be like this:
     *
     * +-------------------------+
     * | struct wiphy       |
     * +-------------------------+
     * | struct ieee80211_local  |
     * +-------------------------+
     * | driver's private data   |
     * +-------------------------+
     *
     */
    priv_size = ALIGN(sizeof(*local), NETDEV_ALIGN) + priv_data_len;
    wiphy = wiphy_new_nm(&mac80211_config_ops, priv_size, requested_name);

    if (!wiphy)
        return NULL;

    wiphy->privid = mac80211_wiphy_privid;

    wiphy->flags |= WIPHY_FLAG_NETNS_OK |
    WIPHY_FLAG_4ADDR_AP |
    WIPHY_FLAG_4ADDR_STATION |
    WIPHY_FLAG_REPORTS_OBSS |
    WIPHY_FLAG_OFFCHAN_TX;

    if (ops->remain_on_channel)
        wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;

    wiphy->features |= NL80211_FEATURE_SK_TX_STATUS |
    NL80211_FEATURE_SAE |
    NL80211_FEATURE_HT_IBSS |
    NL80211_FEATURE_VIF_TXPOWER |
    NL80211_FEATURE_MAC_ON_CREATE |
    NL80211_FEATURE_USERSPACE_MPM |
    NL80211_FEATURE_FULL_AP_CLIENT_STATE;
    wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_FILS_STA);

    if (!ops->hw_scan)
        wiphy->features |= NL80211_FEATURE_LOW_PRIORITY_SCAN |
    NL80211_FEATURE_AP_SCAN;


    if (!ops->set_key)
        wiphy->flags |= WIPHY_FLAG_IBSS_RSN;

    wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_RRM);

    wiphy->bss_priv_size = sizeof(struct ieee80211_bss);

    local = wiphy_priv(wiphy);

    local->hw.wiphy = wiphy;

    local->hw.priv = (char *)local + ALIGN(sizeof(*local), NETDEV_ALIGN);

    local->ops = ops;
    local->use_chanctx = use_chanctx;

    /* set up some defaults */
    local->hw.queues = 1;
    //local->hw.max_ s = 0;
    local->hw.max_report_rates = 0;
    local->hw.max_rx_aggregation_subframes = IEEE80211_MAX_AMPDU_BUF;
    local->hw.max_tx_aggregation_subframes = IEEE80211_MAX_AMPDU_BUF;
    local->hw.offchannel_tx_hw_queue = IEEE80211_INVAL_HW_QUEUE;
    local->hw.conf.long_frame_max_tx_count = 1;
    local->hw.conf.short_frame_max_tx_count =1;
    local->hw.radiotap_mcs_details = IEEE80211_RADIOTAP_MCS_HAVE_MCS |
    IEEE80211_RADIOTAP_MCS_HAVE_GI |
    IEEE80211_RADIOTAP_MCS_HAVE_BW;
    local->hw.radiotap_vht_details = IEEE80211_RADIOTAP_VHT_KNOWN_GI |
    IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH;
    local->hw.uapsd_queues = IEEE80211_DEFAULT_UAPSD_QUEUES;
    local->hw.uapsd_max_sp_len = IEEE80211_DEFAULT_MAX_SP_LEN;
    local->user_power_level = 20;
    local->ext_capa[7] = WLAN_EXT_CAPA8_OPMODE_NOTIF;

    INIT_LIST_HEAD(&local->interfaces);
    INIT_LIST_HEAD(&local->mon_list);
    
    mutex_init(&local->iflist_mtx);
    mutex_init(&local->mtx);

    mutex_init(&local->key_mtx);
    spin_lock_init(&local->filter_lock);
    spin_lock_init(&local->rx_path_lock);
    spin_lock_init(&local->queue_stop_reason_lock);
    local->hw.radiotap_timestamp.units_pos = -1;
    local->hw.radiotap_timestamp.accuracy = -1;
    return &local->hw;
    #else


    if (WARN_ON(!ops->tx || !ops->start || !ops->stop || !ops->config ||
            !ops->add_interface || !ops->remove_interface ||
            !ops->configure_filter))
        return NULL;

    if (WARN_ON(ops->sta_state && (ops->sta_add || ops->sta_remove)))
        return NULL;

    /* check all or no channel context operations exist */
    i = !!ops->add_chanctx + !!ops->remove_chanctx +
        !!ops->change_chanctx + !!ops->assign_vif_chanctx +
        !!ops->unassign_vif_chanctx;
    if (WARN_ON(i != 0 && i != 5))
        return NULL;
    use_chanctx = i == 5;

    /* Ensure 32-byte alignment of our private data and hw private data.
     * We use the wiphy priv data for both our ieee80211_local and for
     * the driver's private data
     *
     * In memory it'll be like this:
     *
     * +-------------------------+
     * | struct wiphy       |
     * +-------------------------+
     * | struct ieee80211_local  |
     * +-------------------------+
     * | driver's private data   |
     * +-------------------------+
     *
     */
    priv_size = ALIGN(sizeof(*local), NETDEV_ALIGN) + priv_data_len;

    wiphy = wiphy_new_nm(&mac80211_config_ops, priv_size, requested_name);

    if (!wiphy)
        return NULL;

    //wiphy->mgmt_stypes = ieee80211_default_mgmt_stypes;

    wiphy->privid = mac80211_wiphy_privid;

    wiphy->flags |= WIPHY_FLAG_NETNS_OK |
            WIPHY_FLAG_4ADDR_AP |
            WIPHY_FLAG_4ADDR_STATION |
            WIPHY_FLAG_REPORTS_OBSS |
            WIPHY_FLAG_OFFCHAN_TX;

    if (ops->remain_on_channel)
        wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;

    wiphy->features |= NL80211_FEATURE_SK_TX_STATUS |
               NL80211_FEATURE_SAE |
               NL80211_FEATURE_HT_IBSS |
               NL80211_FEATURE_VIF_TXPOWER |
               NL80211_FEATURE_MAC_ON_CREATE |
               NL80211_FEATURE_USERSPACE_MPM |
               NL80211_FEATURE_FULL_AP_CLIENT_STATE;

    if (!ops->hw_scan)
        wiphy->features |= NL80211_FEATURE_LOW_PRIORITY_SCAN |
                   NL80211_FEATURE_AP_SCAN;


    if (!ops->set_key)
        wiphy->flags |= WIPHY_FLAG_IBSS_RSN;

    wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_RRM);

    wiphy->bss_priv_size = sizeof(struct ieee80211_bss);

    local = wiphy_priv(wiphy);

    /*if (sta_info_init(local))
        goto err_free;
    */
    local->hw.wiphy = wiphy;

    local->hw.priv = (char *)local + ALIGN(sizeof(*local), NETDEV_ALIGN);

    local->ops = ops;
    local->use_chanctx = use_chanctx;

    /* set up some defaults */
    local->hw.queues = 1;
    local->hw.max_rates = 1;
    local->hw.max_report_rates = 0;
    local->hw.max_rx_aggregation_subframes = IEEE80211_MAX_AMPDU_BUF;
    local->hw.max_tx_aggregation_subframes = IEEE80211_MAX_AMPDU_BUF;
    local->hw.offchannel_tx_hw_queue = IEEE80211_INVAL_HW_QUEUE;
    local->hw.conf.long_frame_max_tx_count = wiphy->retry_long;
    local->hw.conf.short_frame_max_tx_count = wiphy->retry_short;
    local->hw.radiotap_mcs_details = IEEE80211_RADIOTAP_MCS_HAVE_MCS |
                     IEEE80211_RADIOTAP_MCS_HAVE_GI |
                     IEEE80211_RADIOTAP_MCS_HAVE_BW;
    local->hw.radiotap_vht_details = IEEE80211_RADIOTAP_VHT_KNOWN_GI |
                     IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH;
    local->hw.uapsd_queues = IEEE80211_DEFAULT_UAPSD_QUEUES;
    local->hw.uapsd_max_sp_len = IEEE80211_DEFAULT_MAX_SP_LEN;
    local->user_power_level = IEEE80211_UNSET_POWER_LEVEL;
    //wiphy->ht_capa_mod_mask = &mac80211_ht_capa_mod_mask;
    //wiphy->vht_capa_mod_mask = &mac80211_vht_capa_mod_mask;

    local->ext_capa[7] = WLAN_EXT_CAPA8_OPMODE_NOTIF;

    wiphy->extended_capabilities = local->ext_capa;
    wiphy->extended_capabilities_mask = local->ext_capa;
    wiphy->extended_capabilities_len =
        ARRAY_SIZE(local->ext_capa);

    INIT_LIST_HEAD(&local->interfaces);

    __hw_addr_init(&local->mc_list);

    mutex_init(&local->iflist_mtx);
    mutex_init(&local->mtx);

    mutex_init(&local->key_mtx);
    spin_lock_init(&local->filter_lock);
    spin_lock_init(&local->rx_path_lock);
    spin_lock_init(&local->queue_stop_reason_lock);

    INIT_LIST_HEAD(&local->chanctx_list);
    mutex_init(&local->chanctx_mtx);

    //INIT_DELAYED_WORK(&local->scan_work, ieee80211_scan_work);

    /*INIT_WORK(&local->restart_work, ieee80211_restart_work);

    INIT_WORK(&local->radar_detected_work,
          ieee80211_dfs_radar_detected_work);

    INIT_WORK(&local->reconfig_filter, ieee80211_reconfig_filter);
    local->smps_mode = IEEE80211_SMPS_OFF;

    INIT_WORK(&local->dynamic_ps_enable_work,
          ieee80211_dynamic_ps_enable_work);
    INIT_WORK(&local->dynamic_ps_disable_work,
          ieee80211_dynamic_ps_disable_work);*/
    //setup_timer(&local->dynamic_ps_timer,            ieee80211_dynamic_ps_timer, (unsigned long) local);

    /*INIT_WORK(&local->sched_scan_stopped_work,
          ieee80211_sched_scan_stopped_work);

    INIT_WORK(&local->tdls_chsw_work, ieee80211_tdls_chsw_work);*/ 

    spin_lock_init(&local->ack_status_lock);
    idr_init(&local->ack_status_frames);

    for (i = 0; i < IEEE80211_MAX_QUEUES; i++) {
        skb_queue_head_init(&local->pending[i]);
        atomic_set(&local->agg_queue_stop[i], 0);
    }
    /*tasklet_init(&local->tx_pending_tasklet, ieee80211_tx_pending,
             (unsigned long)local);

    tasklet_init(&local->tasklet,
             ieee80211_tasklet_handler,
             (unsigned long) local);
    */
    skb_queue_head_init(&local->skb_queue);
    skb_queue_head_init(&local->skb_queue_unreliable);
    skb_queue_head_init(&local->skb_queue_tdls_chsw);

    //ieee80211_alloc_led_names(local);

    //ieee80211_roc_setup(local);

    local->hw.radiotap_timestamp.units_pos = -1;
    local->hw.radiotap_timestamp.accuracy = -1;
    return &local->hw;
#endif
}
EXPORT_SYMBOL(ieee80211_alloc_hw_nm);

/**
 * @brief      { function_description }
 *
 * @param      hw    The hardware
 * @param      skb   The skb
 */
void ieee80211_tx_status(struct ieee80211_hw *hw, struct sk_buff *skb)
{
    #ifdef DEBUG_VMAC
        printk(KERN_INFO "VMAC_HOOKS: decreasing hdrq\n");
    #endif
  //  dec_hrdq();
    
    if(skb) 
        dev_kfree_skb(skb);
    #ifdef DEBUG_VMAC
        printk(KERN_INFO "VMAC_HOOKS: Done decreasing hdrq\n");
    #endif
}
EXPORT_SYMBOL(ieee80211_tx_status);

void ieee80211_stop_queues(struct ieee80211_hw *hw)
{

}
EXPORT_SYMBOL(ieee80211_stop_queues);

void ieee80211_iterate_active_interfaces_atomic(struct ieee80211_hw *hw, u32 iter_flags,void (*iterator)(void *data, u8 *mac, struct ieee80211_vif *vif),void *data)
{
    
}
EXPORT_SYMBOL_GPL(ieee80211_iterate_active_interfaces_atomic);

struct sk_buff *ieee80211_beacon_get_tim(struct ieee80211_hw *hw,
    struct ieee80211_vif *vif,
    u16 *tim_offset, u16 *tim_length)
{
    return NULL;
}
EXPORT_SYMBOL(ieee80211_beacon_get_tim);

bool ieee80211_csa_is_complete(struct ieee80211_vif *vif)
{
    return true;
}
EXPORT_SYMBOL(ieee80211_csa_is_complete);

void ieee80211_queue_work(struct ieee80211_hw *hw, struct work_struct *work)
{

}
EXPORT_SYMBOL(ieee80211_queue_work);

void ieee80211_csa_finish(struct ieee80211_vif *vif)
{

}
EXPORT_SYMBOL(ieee80211_csa_finish);

void ieee80211_start_tx_ba_cb_irqsafe(struct ieee80211_vif *vif,
    const u8 *ra, u16 tid)
{

}
EXPORT_SYMBOL(ieee80211_start_tx_ba_cb_irqsafe);

struct sk_buff * ieee80211_get_buffered_bc(struct ieee80211_hw *hw,   struct ieee80211_vif *vif)
{
    return NULL;
}
EXPORT_SYMBOL(ieee80211_get_buffered_bc);

void ieee80211_unregister_hw(struct ieee80211_hw *hw)
{
    /* TODO */
}
EXPORT_SYMBOL(ieee80211_unregister_hw);
