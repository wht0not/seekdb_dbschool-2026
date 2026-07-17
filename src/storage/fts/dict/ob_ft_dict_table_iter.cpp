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

#include "storage/fts/dict/ob_ft_dict_table_iter.h"

#include "common/mysqlclient/ob_mysql_proxy.h"
#include "common/mysqlclient/ob_mysql_result.h"
#include "lib/ob_errno.h"
#include "lib/oblog/ob_log_module.h"
#include "lib/string/ob_sql_string.h"
#include "lib/utility/ob_macro_utils.h"
#include "share/ob_server_struct.h"

#define USING_LOG_PREFIX STORAGE_FTS

namespace oceanbase
{
namespace storage
{
ObFTDictTableIter::ObFTDictTableIter(ObISQLClient::ReadResult &result)
    : ObIFTDictIterator(), is_inited_(false), has_current_(false), res_(result)
{
}

int ObFTDictTableIter::get_key(ObString &str)
{
  int ret = OB_SUCCESS;
  if (!IS_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("Not inited.", K(ret));
  } else if (!has_current_) {
    ret = OB_ITER_END;
  } else if (OB_FAIL(res_.get_result()->get_varchar("word", str))) {
    LOG_WARN("Failed to get varchar", K(ret));
  }
  return ret;
}

int ObFTDictTableIter::get_value()
{
  int ret = OB_SUCCESS;
  return ret;
}

int ObFTDictTableIter::next()
{
  int ret = OB_SUCCESS;
  if (!IS_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("Not inited.", K(ret));
  } else if (OB_FAIL(res_.get_result()->next())) {
    has_current_ = false;
    if (OB_ITER_END != ret) {
      LOG_WARN("Failed to get next row", K(ret));
    }
  } else {
    has_current_ = true;
  }
  return ret;
}

int ObFTDictTableIter::init(const ObString &table_name)
{
  int ret = OB_SUCCESS;
  common::ObMySQLProxy *sql_proxy = GCTX.sql_proxy_;

  if (IS_INIT) {
    ret = OB_INIT_TWICE;
    LOG_WARN("Inited twice.", K(ret));
  } else if (table_name.empty()) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("empty table name", K(ret));
  } else if (OB_ISNULL(sql_proxy)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("sql proxy is null", K(ret));
  } else {
    ObString db_name;
    ObString tbl_name;
    const char *dot = static_cast<const char *>(
        memchr(table_name.ptr(), '.', table_name.length()));
    if (OB_ISNULL(dot)) {
      db_name = ObString::make_string("oceanbase");
      tbl_name = table_name;
    } else {
      const int64_t db_len = dot - table_name.ptr();
      db_name.assign_ptr(table_name.ptr(), static_cast<int32_t>(db_len));
      tbl_name.assign_ptr(dot + 1,
                          static_cast<int32_t>(table_name.length() - db_len - 1));
    }

    SMART_VAR(ObSqlString, sql_string)
    {
      if (OB_FAIL(sql_string.append_fmt("SELECT word FROM `%.*s`.`%.*s` ORDER BY word",
                                        db_name.length(),
                                        db_name.ptr(),
                                        tbl_name.length(),
                                        tbl_name.ptr()))) {
        LOG_WARN("Failed to append sql", K(ret), K(db_name), K(tbl_name));
      } else if (OB_FAIL(sql_proxy->read(res_, sql_string.ptr()))) {
        LOG_WARN("Failed to execute sql", K(ret), K(sql_string));
      }
    }

    if (OB_FAIL(ret)) {
      // already logged
    } else if (OB_ISNULL(res_.get_result())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("Failed to get result", K(ret));
    } else if (OB_FAIL(res_.get_result()->next())) {
      if (OB_ITER_END != ret) {
        LOG_WARN("Failed to get next row", K(ret));
      } else {
        // empty dictionary table is allowed
        ret = OB_SUCCESS;
        has_current_ = false;
        is_inited_ = true;
      }
    } else {
      has_current_ = true;
      is_inited_ = true;
    }
  }

  return ret;
}

void ObFTDictTableIter::reset()
{
  res_.close();
  has_current_ = false;
  is_inited_ = false;
}

} //  namespace storage
} //  namespace oceanbase
