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
*/

/* queues */
int queuethread(void* data);
int vmacupperq(void *data);
int rqueuethread(void* data);
int rmgmtqueuethread(void*data);
void queue_init(void);
void queue_start(void);
void add_DACK(struct sk_buff* skb);
void retrx(struct sk_buff* skb, u8 rate);
void enq_usrqueue(struct sk_buff* skb2, char * enc, u8 type, char* seq, char* rate);
void enq_uqueue(struct sk_buff* skb, u16 seq,u8 rate);
void dec_hrdq(void);
void add_mgmt(struct sk_buff* skb);
