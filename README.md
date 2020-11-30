# V-MAC-Kernel
This repository contains V-MAC Kernel Module. @note: ieee802* and sta_info may need to be changed per kernel version.

# Changes to ath9k_htc

instead of calling ieee80211_rx, swap to ieee80211_rx_vmac and create function by hijacking get_stats to check if slot is empty or not, sample function available in ath9k_htc folder.
