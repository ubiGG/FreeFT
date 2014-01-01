/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_NETWORK_H
#define SYS_NETWORK_H

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "base.h"


namespace net {

	u32 resolveName(const char *name);
	void decomposeIp(u32 ip, u8 elems[4]);

	void encodeInt3(Stream&, const int3 &value);
	const int3 decodeInt3(Stream&);

	struct SeqNumber {
	public:
		SeqNumber() { }
		SeqNumber(int i) :value(i) { }
		operator int() const { return (int)value; }
		
		bool operator<(const SeqNumber &rhs) const {
			int a(value), b(rhs.value);
			return (a < b && b - a < 32768 ) ||
				   (a > b && a - b > 32768);
		}
		bool operator>(const SeqNumber &rhs) const { return rhs < *this; }
		bool operator==(const SeqNumber &rhs) { return value == rhs.value; }
		bool operator!=(const SeqNumber &rhs) { return value != rhs.value; }

		const SeqNumber &operator++(int) {
			value++;
			return *this;
		}

		unsigned short value;
	};

	struct Address {
	public:
		Address() :ip(0), port(0) { }
		Address(u32 ip, u16 port) :ip(ip), port(port) { }
		Address(u16 port); // any interface

		bool isValid() const { return ip != 0 || port != 0; }
		bool operator==(const Address &rhs) const { return ip == rhs.ip && port == rhs.port; }

		const string toString() const;

		u32 ip;
		u16 port;
	};


	class Socket {
	public:
		Socket(const Address &address);
		~Socket();
		
		void operator=(const Socket&) = delete;
		Socket(const Socket&) = delete;

		void operator=(Socket&&);
		Socket(Socket&&);

		int receive(char *buffer, int buffer_size, Address &source);
		void send(const char *data, int size, const Address &target);
		void close();

	protected:
		int m_fd;
	};

	struct PacketInfo {
		PacketInfo() { }
		PacketInfo(SeqNumber packet_id, SeqNumber timestamp, int client_id_, int flags_);

		i16 protocol_id;
		SeqNumber packet_id;
		SeqNumber timestamp;
		i8 client_id;
		i8 flags;

		enum {
			max_size = 1400,
			header_size = sizeof(protocol_id) + sizeof(packet_id) + sizeof(timestamp) + sizeof(client_id) + sizeof(flags),
			valid_protocol_id = 0x1234,

			flag_need_ack = 1,
		};

		void save(Stream &sr) const;
		void load(Stream &sr);
	};

	class InPacket: public Stream {
	public:
		InPacket() :Stream(true) { }

		bool end() const { return m_pos == m_size; }
		const PacketInfo &info() const { return m_info; }
		SeqNumber packetId() const { return m_info.packet_id; }
		SeqNumber timestamp() const { return m_info.timestamp; }
		int clientId() const { return m_info.client_id; }
		int flags() const { return m_info.flags; }

		template <class T>
		void skip() {
			T temp;
			*this >> temp;
		}

	protected:
		virtual void v_load(void *ptr, int size) final;
		void ready(int new_size);
		friend class Host;
		
		char m_data[PacketInfo::max_size];
		PacketInfo m_info;
	};

	class OutPacket: public Stream {
	public:
		OutPacket() :Stream(false) { m_size = 0; }
		OutPacket(SeqNumber packet_id, SeqNumber timestamp, int client_id, int flags);

		int spaceLeft() const { return sizeof(m_data) - m_pos; }

	protected:
		virtual void v_save(const void *ptr, int size) final;

		friend class Host;
		char m_data[PacketInfo::max_size];
	};

	//TODO: network data verification (so that the game won't crash under any circumstances)

	class Host {
	public:
		Host(const net::Address &address) :m_socket(address) { }

		bool receive(InPacket &packet, Address &source);
		void send(const OutPacket &packet, const Address &target);

	protected:
		net::Socket m_socket;
	};

	struct JoinAcceptPacket {
		void save(Stream&) const;
		void load(Stream&);

		string map_name;
		i32 actor_id;
	};

	enum class SubPacketType: char {
		join,
		join_accept,
		join_refuse,

		leave,
		ack,

		entity_full,
		entity_delete,
		entity_update,

		actor_order,
	};

	enum class RefuseReason: char {
		too_many_clients,
	};

}

SERIALIZE_AS_POD(net::SeqNumber)

#endif
