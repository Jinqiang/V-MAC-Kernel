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
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/
#include "vmac.h"

struct rate_decision global;
spinlock_t ratelok;
u8 mainrate=0;
int ratebusy;
/**
 * @brief    Initialize frame rate adaptation struct.
 */
void rate_init(void)
{
    global.rate_idx[0]=2;
    global.rate_idx[1]=3;
    global.rate_idx[2]=11;
    global.rate=0;
    ratebusy=0;
    mainrate=0;
    spin_lock_init(&ratelok);
}

/**
 * @brief    copies data rates adjustments to a local memory for function passing pointer.
 *
 * @param      rates    The rates
 */
void copyrate(u8 *rates)
{
    if(spin_trylock(&ratelok))
    {
        *rates=global.rate_idx[0];
        *(rates+1)=global.rate_idx[1];
        *(rates+2)=global.rate_idx[2];
        spin_unlock(&ratelok);
    }
}
/**
 * @brief    Rate control Algorithm
 *
 * @param      k    last reported's DACK holes missing frames rates indicator
 *             (i.e. which frame rates led to frame loss)
 */
void rate_c(struct missing_idx* k)
{
    int high,low;
    if(!spin_trylock(&ratelok))
        return;
    //struct missing_idx* k=&mis;
    high=k->round*5;
    low=k->round*5;
    high=high/3;
    low=low/3;
    high=(int)(k->round*7/10);
    low=(int)(k->round/2);
    //decreasing rate
    if(k->rate[0]>low&&global.rate_idx[0]>0&&k->round_dec[0]<k->round)
    {
        global.rate_idx[0]--;
        k->round_dec[0]=k->round;
    }
    if(k->rate[1]>low&&global.rate_idx[1]-1>global.rate_idx[0]&&k->round_dec[1]<k->round)
    {
        global.rate_idx[1]--;
        k->round_dec[1]=k->round;
    }
    if(k->rate[2]>low&&global.rate_idx[2]-1>global.rate_idx[1]&&k->round_dec[2]<k->round)
    {
        
        global.rate_idx[2]--;
        if(global.rate_idx[2]==34)global.rate_idx[2]=11;
        k->round_dec[2]=k->round;
    }
    
    
    //increasing rate
    if(k->rate[2]<high&&global.rate_idx[2]<35&&k->round_inc[2]+2<k->round&&k->round!=k->round_dec[2])
    {
        global.rate_idx[2]++;
        if(global.rate_idx[2]==12)global.rate_idx[2]=35;
        k->round_inc[2]=k->round;
    }
    if(k->rate[1]<high&&global.rate_idx[1]<global.rate_idx[2]-1&&global.rate_idx[1]<11&&k->round_inc[1]+2<k->round&&k->round!=k->round_dec[1])
    {
        global.rate_idx[1]++;
        k->round_inc[1]=k->round;
    }
    if(k->rate[0]<high&&global.rate_idx[0]<global.rate_idx[1]-1&&global.rate_idx[0]<10&&k->round_inc[0]+2<k->round&&k->round!=k->round_dec[0])
    {
        global.rate_idx[0]++;
        k->round_inc[0]=k->round;
    }
    spin_unlock(&ratelok);
}
