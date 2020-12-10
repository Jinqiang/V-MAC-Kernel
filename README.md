# V-MAC-Kernel
This repository contains V-MAC Kernel Module. @note: ieee802* and sta_info may need to be changed per kernel version.

# Changes to ath9k_htc

instead of calling ieee80211_rx, swap to ieee80211_rx_vmac and create function by hijacking get_stats to check if slot is empty or not, sample function available in ath9k_htc folder.

## Reference

Please cite work work while using the system, Thank you!

Paper: Pub/Sub in the Air: A Novel Data-centric Radio Supporting Robust Multicast in Edge Environments

bibtex:

@inproceedings{vmac,
  title={Pub/Sub in the Air: A Novel Data-centric Radio Supporting Robust Multicast in Edge Environments.},
  author={Elbadry, Mohammed and Ye, Fan and Milder, Peter and Yang, YuanYuan},
  booktitle={Proceedings of the 5th ACM/IEEE Symposium on Edge Computing},
  pages={1--14},
  year={2020}
}
