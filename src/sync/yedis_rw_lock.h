/*
 *
 *unfair Reader-Writer lock implementation with reader preferment .
 *If a thread can not get lock, it will spin.
 */
#ifndef YEDIS_RW_LOCK_H_
#define YEDIS_RW_LOCK_H_
#include "../base/yedis_common.h"
namespace yedis_sync
{
  class YedisRWLock
  {
  public:
    YedisRWLock()
    {
      lock_ = 0;
    }
    bool lockForWriter()
    {
      while(true) {
        if(CAS(&lock_, 0, 1))
          return true;
      }
    }
    bool lockForReader(uint8_t reader_id)
    {
      //1 <= reader_id <= 63
      bool ret = false;
      if (reader_id <= 63 && reader_id > 0) {
        while(true) {
          uint64_t tmp = ACCESS_ONCE(lock_);
          uint64_t mask = 1ULL << reader_id;
          if (!(tmp & 1)) {
            //no writer
            mask |= tmp;
            if (CAS(&lock_, tmp, mask)) {
              ret = true;
              break;
            }
          }
        }
      }
      return ret;
    }
    bool unlockForReader(uint8_t reader_id)
    {
      //1 <= reader_id <= 63
      bool ret = false;
      if (reader_id <= 63 && reader_id > 0) {
        while(true) {
          uint64_t tmp = ACCESS_ONCE(lock_);
          uint64_t mask = 1ULL << reader_id;
          if (!(tmp & 1)) {
            //no writer
            mask = (~mask) & tmp;
            if (CAS(&lock_, tmp, mask)) {
              ret = true;
              break;
            }
          }
        }
      }
      return ret;
    }
    bool unlockForWriter()
    {
      return CAS(&lock_, 1, 0);
    }
  private:
    uint64_t lock_;
  };
  class YedisReaderLockGuard
  {
  public:
    YedisReaderLockGuard(YedisRWLock &lock, uint8_t reader_id):lock_(lock),reader_id_(reader_id)
    {
      lock_.lockForReader(reader_id_);
    }
    ~YedisReaderLockGuard()
    {
      lock_.unlockForReader(reader_id_);
    }
  private:
    YedisRWLock &lock_;
    uint8_t reader_id_;
  };
  class YedisWriterLockGuard
  {
  public:
    YedisWriterLockGuard(YedisRWLock &lock):lock_(lock)
    {
      lock_.lockForWriter();
    }
    ~YedisWriterLockGuard()
    {
      lock_.unlockForWriter();
    }
  private:
    YedisRWLock &lock_;
  };
}
#endif
