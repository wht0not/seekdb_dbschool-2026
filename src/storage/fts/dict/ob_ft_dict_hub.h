/*
 * Copyright (c) 2025 OceanBase.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _OCEANBASE_STORAGE_FTS_DICT_OB_FT_DICT_HUB_H_
#define _OCEANBASE_STORAGE_FTS_DICT_OB_FT_DICT_HUB_H_

#include "lib/charset/ob_charset.h"
#include "lib/hash_func/murmur_hash.h"
#include "lib/lock/ob_bucket_lock.h"
#include "lib/string/ob_string.h"
#include "storage/fts/dict/ob_ft_dict_def.h"

namespace oceanbase
{
namespace storage
{

class ObFTDictInfo
{
public:
  ObFTDictInfo()
      : name_(""),
        type_(ObFTDictType::DICT_TYPE_INVALID),
        charset_(CHARSET_INVALID),
        version_(0),
        range_count_(0)
  {
  }

public:
  char name_[2048]; // for now
  ObFTDictType type_;
  ObCharsetType charset_;
  int64_t version_; // in memory
  int32_t range_count_;
};

struct ObFTDictInfoKey
{
public:
  ObFTDictInfoKey()
      : type_(static_cast<uint64_t>(ObFTDictType::DICT_TYPE_INVALID)), name_(0)
  {
  }
  ObFTDictInfoKey(const uint64_t type, const uint64_t name = 0)
      : type_(type), name_(name)
  {
  }
  int hash(uint64_t &hash_value) const
  {
    int ret = OB_SUCCESS;
    hash_value = hash();
    return ret;
  }

  uint64_t hash() const
  {
    uint64_t hash_val = 0;
    hash_val = common::murmurhash(&type_, sizeof(type_), hash_val);
    hash_val = common::murmurhash(&name_, sizeof(name_), hash_val);
    return hash_val;
  }

  bool operator==(const ObFTDictInfoKey &other) const
  {
    return type_ == other.type_ && name_ == other.name_;
  }

  int compare(const ObFTDictInfoKey &other) const
  {
    int ret = 0;
    if (type_ != other.type_) {
      ret = type_ < other.type_ ? -1 : 1;
    } else if (name_ != other.name_) {
      ret = name_ < other.name_ ? -1 : 1;
    }
    return ret;
  }

private:
  uint64_t type_;
  uint64_t name_;
};

class ObFTCacheRangeContainer;
class ObFTDictHub
{
public:
  ObFTDictHub() : is_inited_(false), dict_map_(), version_map_(), rw_dict_lock_() {}
  ~ObFTDictHub() {}

  int init();

  int destroy();

  int build_cache(const ObFTDictDesc &desc, ObFTCacheRangeContainer &container);

  int load_cache(const ObFTDictDesc &desc, ObFTCacheRangeContainer &container);

  // Bump in-memory version so subsequent loads rebuild from the table.
  int invalidate_dict(const common::ObString &qualified_table_name);

  int get_dict_version(const common::ObString &qualified_table_name, int64_t &version);

private:
  int get_dict_info(const ObFTDictInfoKey &key, ObFTDictInfo &info);

  int put_dict_info(const ObFTDictInfoKey &key, const ObFTDictInfo &info);

  static uint64_t calc_name_hash(const common::ObString &name);

private:
  bool is_inited_;
  // holds info of dict
  hash::ObHashMap<ObFTDictInfoKey, ObFTDictInfo> dict_map_;
  // qualified table name hash -> version (for REFRESH)
  hash::ObHashMap<uint64_t, int64_t> version_map_;
  ObBucketLock rw_dict_lock_;
};

} //  namespace storage
} //  namespace oceanbase

#endif // _OCEANBASE_STORAGE_FTS_DICT_OB_FT_DICT_HUB_H_
