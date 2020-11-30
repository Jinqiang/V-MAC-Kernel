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

struct missing_idx {
	u8 rate[3];
	u8 round;
	u8 *round_inc;
	u8 *round_dec;
};

/* rate control algorithm function (NOTE: THIS IS PRELIMINARY)*/ 
void rate_c(struct missing_idx* k);
void rate_init(void);
void copyrate(u8 *rates);
