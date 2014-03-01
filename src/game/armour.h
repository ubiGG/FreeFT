/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef FREEFT_GAME_ARMOUR_H
#define FREEFT_GAME_ARMOUR_H

#include "game/item.h"


namespace game {

	struct ArmourProto: public ProtoImpl<ArmourProto, ItemProto, ProtoId::item_armour> {
		ItemType::Type itemType() const { return ItemType::armour; }
		ArmourProto(const TupleParser&);

		ArmourClassId::Type class_id;
		float damage_resistance;
		string sound_prefix;
	};

	struct Armour: public Item
	{
	public:
		Armour(const Item &item) :Item((DASSERT(item.type() == ItemType::armour), item)) { }
		Armour(const ArmourProto &proto) :Item(proto) { }

		ArmourClassId::Type classId() const		{ return proto().class_id; }

		const ArmourProto &proto() const		{ return static_cast<const ArmourProto&>(*m_proto); }
	};

}

#endif