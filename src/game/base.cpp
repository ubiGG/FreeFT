/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/base.h"
#include "audio/device.h"
#include <cstring>

namespace game {

	static const EnumMap<HealthStatus, const char*> s_health_desc = {
		"unhurt",
		"barely wounded",
		"wounded",
		"seriously wounded",
		"near death",
		"dead"
	};

	const char *describe(HealthStatus status) {
		return s_health_desc[status];
	}

	HealthStatus healthStatusFromHP(float health) {
			return
				health > 0.99f? HealthStatus::unhurt :
				health > 0.70f? HealthStatus::barely_wounded :
				health > 0.50f? HealthStatus::wounded :
				health > 0.20f? HealthStatus::seriously_wounded :
				health > 0.0f ? HealthStatus::near_death : HealthStatus::dead;
		}

	namespace AttackModeFlags {
		uint fromString(const char *string) {
			return ::toFlags(string, enumStrings(AttackMode()), 1);
		}

		Maybe<AttackMode> getFirst(uint flags) {
			for(int n = 0; n < count<AttackMode>(); n++)
				if(flags & (1 << n))
					return (AttackMode)n;
			return none;
		}
	};
	
	SoundId::SoundId(const char *sound_name, int offset) {
		audio::SoundIndex index = audio::findSound(sound_name);
		m_id = (int)index + (offset < 0 || offset > index.variation_count? 0 : offset);
	}
}
