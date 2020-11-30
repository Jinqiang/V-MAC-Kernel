static int ath9k_htc_get_stats(struct ieee80211_hw *hw,
			       struct ieee80211_low_level_stats *stats)
{
		int slot;
	struct ath9k_htc_priv *priv = hw->priv;
	spin_lock_bh(&priv->tx.tx_lock);
	slot = find_first_zero_bit(priv->tx.tx_slot, MAX_TX_BUF_NUM);
	if (slot >= MAX_TX_BUF_NUM) {
		spin_unlock_bh(&priv->tx.tx_lock);
		return -1; /* No slots available DO NOT SEND PACKETS DOWN! */
	}
	spin_unlock_bh(&priv->tx.tx_lock);
	return 0; /* slots available you may pass frame */

}