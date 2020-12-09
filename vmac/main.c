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
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
* 
*/
#include "vmac.h"
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Mohammed Elbadry");
MODULE_DESCRIPTION("V-MAC: Pub/Sub multicast MAC layer");
MODULE_VERSION("4.5");

DECLARE_HASHTABLE(rx_enc, 5);
DECLARE_HASHTABLE(tx_enc, 5);

struct sock * nl_sk=NULL;
const struct cfg80211_ops mac80211_config_ops = {
};//just a hack to build and integrate within kernel...


int pidt;
u32 firs;
u32 last;

/**
 * @brief      Netlink Receive from userspace function
 *
 * @param      skb received frame from userspace
 * 
 */
static void nl_recv(struct sk_buff* skb)
{
    struct nlmsghdr *nlh;
    struct sk_buff* skb2;
    struct control rxc;
    u64 enc;
    u8 type;
    int size;
    nlh  = (struct nlmsghdr *) skb->data;
    type = nlh->nlmsg_type;
    if (pidt != nlh->nlmsg_pid) 
        pidt=nlh->nlmsg_pid;

    if (type == VMAC_HDR_INTEREST || type == VMAC_HDR_DATA || type == VMAC_HDR_ANOUNCMENT || type == VMAC_HDR_INJECTED){
        skb2 = dev_alloc_skb(nlh->nlmsg_len + 50);// -10 was here
        size = nlh->nlmsg_len-100;
        memcpy(&rxc, nlmsg_data(nlh), sizeof(struct control));
        if (skb2 == NULL)
        {
            printk(KERN_INFO "VMAC_ERROR: FAILED TO ALLOCATE Memory\n");
            return;
        }
        //skb_reserve(skb2, 90);
        enc = (*(uint64_t*)(rxc.enc));
        memcpy(skb_put(skb2, size), nlmsg_data(nlh) + sizeof(struct control), size);
        enq_usrqueue(skb2, &rxc.enc[0], type, &rxc.seq[0], &rxc.rate[0]); //UNCOMMENT THIS LATER
    }
    else if (type == 255) printk(KERN_INFO "VMAC-upper: userspace PID Registered\n");
    else
    {
        printk(KERN_INFO "ERROR: Unknown type of frame, discarding, please contact author\n");
    }
}
/**
 * @brief      returns PID of userspace process
 *
 * @return     PID (int)
 */
int getpidt(void)
{
    return pidt;
}


/**
 * @brief      returns socket to userspace (netlink socket)
 *
 * @return     socket
 */
struct sock* getsock(void)
{
    return nl_sk;
}


struct encoding_tx* find_tx(int table, u64 enc)
{
    struct encoding_tx *vmact, *ret = NULL;
    hash_for_each_possible(tx_enc, vmact, node, enc)
    {
        ret = vmact;
    }
    return ret;
}
void add_rx(struct encoding_rx *vmacr)
{
    hash_add(rx_enc, &vmacr->node, vmacr->key);
}

void add_tx(struct encoding_tx *vmact)
{
    hash_add(tx_enc, &vmact->node, vmact->key);
}

struct encoding_rx* find_rx(int table, u64 enc)
{
    struct encoding_rx *vmacr, *ret = NULL;
    hash_for_each_possible(rx_enc, vmacr, node, enc)
    {
        ret = vmacr;
    }
    return ret;
}


/**
 * @brief      Initialize V-MAC and start kthreads
 *
 * @return     0 
 */
static int __init vmac_init(void)
{
    struct netlink_kernel_cfg cfg = {.input=nl_recv};
    
    /* init tx and rx hashtables*/
    hash_init(tx_enc);
    hash_init(rx_enc);
    /* init queus, rate adaptation, dack, etc...*/
    queue_init();
    //rate_init();
    dack_init();
    
    pidt = -1;
    /* creating queue and functions threads */
    queue_start();
    dack_start();
    
    nl_sk = netlink_kernel_create(&init_net, VMAC_USER, &cfg);  
    if (!nl_sk)
    {
        printk(KERN_ALERT "VMAC FAILED ERROR: Please contact author\n");
        return -1;
    }

    printk(KERN_INFO "VMAC: Installed sucessfully.\n"); 
    return 0;
}



/**
 * @brief      Stops all threads and empties all memory allocated
 *              TODO: Needs to be completed. 
 */
static void __exit vmac_exit(void)
{
    netlink_kernel_release(nl_sk);
}


module_init(vmac_init);
module_exit(vmac_exit);
