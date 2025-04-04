/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MAC80211_DEBUGFS_KEY_H
#define __MAC80211_DEBUGFS_KEY_H

#ifdef CPTCFG_MAC80211_DEBUGFS
static void ieee80211_debugfs_key_add(struct ieee80211_key *key);
static void ieee80211_debugfs_key_remove(struct ieee80211_key *key);
static void ieee80211_debugfs_key_update_default(struct ieee80211_sub_if_data *sdata);
static void ieee80211_debugfs_key_add_mgmt_default(
	struct ieee80211_sub_if_data *sdata);
static void ieee80211_debugfs_key_remove_mgmt_default(
	struct ieee80211_sub_if_data *sdata);
static void ieee80211_debugfs_key_add_beacon_default(
	struct ieee80211_sub_if_data *sdata);
static void ieee80211_debugfs_key_remove_beacon_default(
	struct ieee80211_sub_if_data *sdata);
static void ieee80211_debugfs_key_sta_del(struct ieee80211_key *key,
				   struct sta_info *sta);
#else
static inline void ieee80211_debugfs_key_add(struct ieee80211_key *key)
{}
static inline void ieee80211_debugfs_key_remove(struct ieee80211_key *key)
{}
static inline void ieee80211_debugfs_key_update_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void ieee80211_debugfs_key_add_mgmt_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void ieee80211_debugfs_key_remove_mgmt_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void ieee80211_debugfs_key_add_beacon_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void ieee80211_debugfs_key_remove_beacon_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void ieee80211_debugfs_key_sta_del(struct ieee80211_key *key,
						 struct sta_info *sta)
{}
#endif

#endif /* __MAC80211_DEBUGFS_KEY_H */
