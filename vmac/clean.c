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
/* cleanup */
/**
 * @brief      Clean up encoding from encoding table (occurs per encoding timeout)
 *
 * @param[in]  data  The data: pointer to encoding cleanup struct
 * 
 * Pseudo Code
 * @code{.unparsed}
 * check type of struct (i.e. tx or rx)
 * if type is ENC_TX
 *  search for encoding at tx table (sanity check)
 *  if not found
 *      return
 *  remove from hastable
 *  free tx_struct
 * else (i.e. type must be RX_ENC)
 *  search for encoding at rx table (sanity check)
 *  if not found
 *      return
 *  for i=0 to either end of retransmission buffer size or latest sequence number transmitted (whichever smaller)
 *      free retransmission buffer frame
 *  remove from hastable
 *  free rx_struct
 *  
 *  @endcode
 */

void process (struct enc_cleanup* clean)
{
    struct encoding_rx* vmacr;
    struct encoding_tx* vmact;
    int i;
    if(clean->type == CLEAN_ENC_RX)
    {
        #ifdef DEBUG_MO
            printk(KERN_INFO "CLEAN: starting process \n");
        #endif
        vmacr= find_rx(RX_TABLE, clean->enc);
        {

        }
        if(!vmacr||vmacr==NULL)
        {
            return;
        }
        #ifdef DEBUG_VMAC
            printk(KERN_INFO "CLEAN: removing element\n");
        #endif        
        hash_del(&vmacr->node);

        if(vmacr->dac_info.dack)
            kfree_skb(vmacr->dac_info.dack);
        vfree(vmacr);
    }
    else /* must be CLEAN_ENC_TX*/
    {
        vmact = find_tx(TX_TABLE, clean->enc);
        {

        }
        if(!vmact||vmact==NULL)
        {
            return;
        }
        #ifdef DEBUG_MO
            printk(KERN_INFO "VMAC_CLEAN: tx emptying buffer\n");
        #endif
        //mutex_lock(&vmact->mt);
        for(i=0;i<(vmact->seq<WINDOW_TX?vmact->seq:WINDOW_TX);i++)
        {
            if(vmact->retransmission_buffer[i])
               kfree_skb(vmact->retransmission_buffer[i]);
        }
        //mutex_unlock(&vmact->mt);
        hash_del(&vmact->node);
        vfree(vmact);
    }
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,0)
void __cleanup_rx(struct timer_list *t)
{
    struct encoding_rx* vmacr = from_timer(vmacr, t, enc_timeout);
    //printk(KERN_INFO "WHY WHY WHY WHY %u \n",vmacr->key);
    if(vmacr)
    {
        #ifdef DEBUG_VMAC
            printk(KERN_INFO"VMAC_CLEAN: rx cleaning started \n");
        #endif
        vmacr->clean.type = CLEAN_ENC_RX;
        process(&vmacr->clean);
    }
    
}
void __cleanup_tx(struct timer_list *t)
{    
    struct encoding_tx* vmact = from_timer(vmact, t, enc_timeout);
    if(vmact)
    {
        vmact->clean.type = CLEAN_ENC_TX;
        process(&vmact->clean);            
    }
}
#else
void __cleanup(unsigned long data)
{
    struct enc_cleanup* clean=(struct enc_cleanup*)data;
    process(clean);
}
#endif
