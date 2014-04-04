/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/world.h"
#include "tile_map.h"
#include "sys/xml.h"
#include "sys/profiler.h"
#include "game/tile.h"
#include "navi_heightmap.h"
#include "audio/device.h"
#include "game/actor.h"
#include <algorithm>

namespace game {

	World::World(const string &file_name, Mode mode, Replicator *replicator)
		:m_mode(mode), m_last_anim_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0),
		m_anim_frame(0), m_tile_map(m_level.tile_map), m_entity_map(m_level.entity_map),
	   	m_replicator(replicator) {
		DASSERT(m_mode == Mode::single_player || m_replicator != nullptr);

		m_level.load(file_name.c_str());
		for(int n = 0; n < m_entity_map.size(); n++) {
			auto &obj = m_entity_map[n];
			if(obj.ptr)
				obj.ptr->hook(this, n);
		}

		int agent_sizes[] = { 3, 4, 7 };
		for(int n = 0; n < COUNTOF(agent_sizes); n++)
			m_navi_maps.emplace_back(agent_sizes[n]);

//		m_tile_map.printInfo();
		m_map_name = file_name;
		updateNaviMap(true);
	}

	World::~World() {
	}
		
	void World::updateNaviMap(bool full_recompute) {
		PROFILE("updateNaviMap");

		if(full_recompute) {
			vector<IBox> bboxes;
			bboxes.reserve(m_tile_map.size() + m_entity_map.size());

			for(int n = 0; n < m_tile_map.size(); n++)
				if(m_tile_map[n].ptr)
					bboxes.push_back((IBox)m_tile_map[n].bbox);
			for(int n = 0; n < m_entity_map.size(); n++)
				if(m_entity_map[n].ptr && (m_entity_map[n].flags & collider_static))
					bboxes.push_back(enclosingIBox(m_entity_map[n].ptr->boundingBox()));

			NaviHeightmap heightmap(m_tile_map.dimensions());
			heightmap.update(bboxes);
			//heightmap.saveLevels();
			//heightmap.printInfo();

			for(int m = 0; m < (int)m_navi_maps.size(); m++) {
				m_navi_maps[m].update(heightmap);
				//m_navi_maps[m].printInfo();
			}
		}

		for(int m = 0; m < (int)m_navi_maps.size(); m++) {
			NaviMap &navi_map = m_navi_maps[m];
			navi_map.removeColliders();

			for(int n = 0; n < m_entity_map.size(); n++) {
				auto &object = m_entity_map[n];
				if(!object.ptr)
					continue;

				if(object.flags & collider_dynamic) {
					const IBox &box = enclosingIBox(object.ptr->boundingBox());
					navi_map.addCollider(box, n);
				}
			}
			navi_map.updateReachability();
		}
	}

	void World::removeEntity(EntityRef ref) {
		if( Entity *entity = refEntity(ref) ) {
			m_entity_map.remove(ref.index());
			replicate(ref.index());
		}
	}

	EntityRef World::addEntity(PEntity &&ptr, int index) {
		DASSERT(ptr);
		Entity *entity = ptr.get();
		index = m_entity_map.add(std::move(ptr), index);
		entity->hook(this, index);
		replicate(index);

		return entity->ref();
	}

	void World::simulate(double time_diff) {
		//TODO: synchronizing time between client/server

		DASSERT(time_diff > 0.0);
		double max_time_diff = 1.0; //TODO: add warning?
		time_diff = min(time_diff, max_time_diff);

		double current_time = m_last_time + time_diff; //TODO: rozjedzie sie z getTime(), ale czy to jest problem?
		double frame_diff = current_time - m_last_anim_frame_time;
		double frame_time = 1.0 / 15.0, fps = 15.0;
		m_current_time = current_time;
		m_time_delta = time_diff;

		int frame_skip = 0;
		if(frame_diff > frame_time) {
			frame_skip = (int)(frame_diff * fps);
			m_last_anim_frame_time += (double)frame_skip * frame_time;
		}
		m_anim_frame += frame_skip;
		if(m_anim_frame < 0)
			m_anim_frame = 0;
		Tile::setFrameCounter(m_anim_frame);

		for(int n = 0; n < m_entity_map.size(); n++) {
			auto &object = m_entity_map[n];
			if(!object.ptr)
				continue;

			object.ptr->think();
			if(object.flags & (collider_dynamic | collider_projectile))
				m_entity_map.update(n);

			for(int f = 0; f < frame_skip; f++)
				object.ptr->nextFrame();
		}

		for(int n = 0; n < (int)m_replace_list.size(); n++) {
			auto &pair = m_replace_list[n];
			int index = pair.second;
			int old_uid = -1;

			if(index != -1) {
				const Entity *entity = m_entity_map[index].ptr;
				if(entity)
					old_uid = entity->m_unique_id;
				removeEntity(index);
			}

			if(pair.first.get()) {
				if(old_uid != -1)
					pair.first.get()->m_unique_id = old_uid;
				index = addEntity(std::move(pair.first), index).index();
			}
			replicate(index);
		}
		m_replace_list.clear();

		m_last_time = current_time;
		updateNaviMap(false);
	}
	
	const Grid::ObjectDef *World::refDesc(ObjectRef ref) const {
		if(ref.isTile()) {
		   	if(ref.m_index >= 0 && ref.m_index < m_tile_map.size())
				return (const Grid::ObjectDef*)&m_tile_map[ref.m_index];
		}
		else {
		   	if(ref.m_index >= 0 && ref.m_index < m_entity_map.size())
				return (const Grid::ObjectDef*)&m_entity_map[ref.m_index];
		}

		return nullptr;
	}
	
	const FBox World::refBBox(ObjectRef ref) const {
		if(ref.isTile()) {
		   	if(ref.m_index >= 0 && ref.m_index < m_tile_map.size())
				return m_tile_map[ref.m_index].bbox;
		}
		else {
		   	if(ref.m_index >= 0 && ref.m_index < m_entity_map.size())
				return m_entity_map[ref.m_index].bbox;
		}

		return FBox::empty();
	}

	const Tile *World::refTile(ObjectRef ref) const {
		if(ref.isTile() && ref.m_index >= 0 && ref.m_index < m_tile_map.size())
			return m_tile_map[ref.m_index].ptr;
		return nullptr;
	}
	
	Entity *World::refEntity(EntityRef ref) {
		if(ref.m_index < 0 || ref.m_index >= m_entity_map.size())
			return nullptr;

		Entity *entity = m_entity_map[ref.m_index].ptr;
		if(entity && (ref.m_unique_id == -1 || entity->m_unique_id == ref.m_unique_id))
			return entity;

		return nullptr;
	}

	Intersection World::trace(const Segment &segment, const Entity *ignore, int flags) const {
		PROFILE("World::trace");
		Intersection out;

		if(flags & collider_tiles) {
			pair<int, float> isect = m_tile_map.trace(segment, -1, flags);
			if(isect.first != -1) {
				const auto &obj = m_tile_map[isect.first];
				out = Intersection(ObjectRef(isect.first, false), isect.second);
			}
		}

		if(flags & collider_entities) {
			pair<int, float> isect = m_entity_map.trace(segment, ignore? ignore->index() : -1, flags);
			if(isect.first != -1 && isect.second <= out.distance()) {
				const auto &obj = m_entity_map[isect.first];
				out = Intersection(ObjectRef(isect.first, true), isect.second);
			}
		}

		return out;
	}

	ObjectRef World::findAny(const FBox &box, const Entity *ignore, ColliderFlags flags) const {
		if((flags & collider_tiles)) {
			int index = m_tile_map.findAny(box, flags);
			if(index != -1)
				return ObjectRef(index, false);
		}

		if(flags & collider_entities) {
			int index = m_entity_map.findAny(box, ignore? ignore->index() : -1, flags);
			if(index != -1)
				return ObjectRef(index, true);
		}

		return ObjectRef();
	}

	void World::findAll(vector<ObjectRef> &out, const FBox &box, const Entity *ignore, ColliderFlags flags) const {
		vector<int> inds;
		inds.reserve(1024);

		if(flags & collider_tiles) {
			m_tile_map.findAll(inds, box, flags);
			for(int n = 0; n < (int)inds.size(); n++)
				out.emplace_back(ObjectRef(inds[n], false));
			inds.clear();
		}

		if(flags & collider_entities) {
			m_entity_map.findAll(inds, box, ignore? ignore->index() : -1, flags);
			for(int n = 0; n < (int)inds.size(); n++)
				out.emplace_back(ObjectRef(inds[n], true));
		}
	}

	bool World::isInside(const FBox &box) const {
		return m_tile_map.isInside(box);
	}
		
	const NaviMap *World::naviMap(int agent_size) const {
		for(int n = 0; n < (int)m_navi_maps.size(); n++)
			if(m_navi_maps[n].agentSize() == agent_size)
				return &m_navi_maps[n];

		printf("not for: %d\n", agent_size);
		return nullptr;
	}
		
	const NaviMap *World::accessNaviMap(const FBox &bbox) const {
		int agent_size = round(max(bbox.width(), bbox.depth()));
		return naviMap(agent_size);
	}
		
	bool World::findClosestPos(int3 &out, const int3 &source, const IBox &target_box, EntityRef agent_ref) const {
		const Entity *agent = const_cast<World*>(this)->refEntity(agent_ref);
		if(agent) {
			FBox bbox = agent->boundingBox();
			const NaviMap *navi_map = accessNaviMap(bbox);
			if(navi_map)
				return navi_map->findClosestPos(out, source, bbox.height(), target_box);
		}

		return false;
	}
		
	bool World::findPath(Path &out, const int3 &start, const int3 &end, EntityRef agent_ref) const {
		const Entity *agent = const_cast<World*>(this)->refEntity(agent_ref);
		if(agent) {
			const NaviMap *navi_map = accessNaviMap(agent->boundingBox());
			if(navi_map) {
				vector<int3> path;
				if(navi_map->findPath(path, start, end, agent_ref.index())) {
					out = Path(path);
					return true;
				}
			}
		}

		return false;
	}
	
	void World::replicate(const Entity *entity) {
		DASSERT(entity && entity->isHooked());
		DASSERT(entity->m_world == this);
		replicate(entity->index());
	}

	void World::replicate(int index) {
		if(isServer())
			m_replicator->replicateEntity(index);
	}

	void World::playSound(const char *name, const float3 &pos) {
		if(isServer())
			return;
		audio::playSound(name, pos);
	}

	void World::playSound(SoundId sound_id, const float3 &pos) {
		if(isServer())
			return;
		audio::playSound(sound_id, pos);
	}

	bool World::sendOrder(POrder &&order_ptr, EntityRef actor_ref) {
		DASSERT(order_ptr);

		if(isClient()) {
			m_replicator->replicateOrder(std::move(order_ptr), actor_ref);
			return true;
		}

		if( Actor *entity = refEntity<Actor>(actor_ref) )
			return entity->setOrder(std::move(order_ptr));
		return false;
	}

}
