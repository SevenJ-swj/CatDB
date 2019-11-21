#ifndef PACKET_H
#define PACKET_H

#include "packet_header.h"

namespace CatDB
{
	namespace Server
	{
		// StatusFlags http://dev.mysql.com/doc/internals/en/status-flags.html
		static const uint16_t SERVER_STATUS_IN_TRANS = 0x0001; // a transaction is active
		static const uint16_t SERVER_STATUS_AUTOCOMMIT = 0x0002; // auto-commit is enabled
		static const uint16_t SERVER_MORE_RESULTS_EXISTS = 0x0008;
		static const uint16_t SERVER_STATUS_NO_GOOD_INDEX_USED = 0x0010;
		static const uint16_t SERVER_STATUS_NO_INDEX_USED = 0x0020;
		static const uint16_t SERVER_STATUS_CURSOR_EXISTS = 0x0040;
		static const uint16_t SERVER_STATUS_LAST_ROW_SENT = 0x0080;
		static const uint16_t SERVER_STATUS_DB_DROPPED = 0x0100;
		static const uint16_t SERVER_STATUS_NO_BACKSLASH_ESCAPES = 0x0200;
		static const uint16_t SERVER_STATUS_METADATA_CHANGED = 0x0400;
		static const uint16_t SERVER_QUERY_WAS_SLOW = 0x0800;
		static const uint16_t SERVER_PS_OUT_PARAMS = 0x1000;

		class Packet
		{
		public:
			Packet() { }
			virtual ~Packet() { }
			virtual int encode(char* buffer, int64_t length, int64_t& pos);
		public:
			const static int64_t MAX_STORE_LENGTH = 9; /* max bytes needed when store_length */

		public:
			virtual int serialize(char* buffer, int64_t len, int64_t& pos) = 0;

			/**
			*  不是准确的序列化(packey payload)需要的长度
			*  为了简化计算 变长序列化数字的时候都假设需要9bytes
			*  不包括MySQL包头需要的4bytes
			*/
			virtual uint64_t get_serialize_size() = 0;

			void set_seq(uint8_t seq)
			{
				header_.seq_ = seq;
			}
		private:
			PacketHeader header_;
		};
	}
}
#endif	//PACKET_H
