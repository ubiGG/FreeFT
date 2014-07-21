/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/controller.h"
#include "io/input.h"
#include "sys/profiler.h"

#include "game/actor.h"
#include "game/item.h"
#include "game/visibility.h"
#include "game/game_mode.h"
#include "game/pc_controller.h"

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"
#include "audio/device.h"

#include "gfx/texture.h"
#include "hud/console.h"
#include "hud/hud.h"
#include "hud/target_info.h"

using namespace gfx;
using namespace game;

namespace io {

	static const float s_exit_anim_length = 0.7f;
	static const float2 target_info_size(200.0f, 80.0f);

	Controller::Controller(const int2 &resolution, PWorld world, bool debug_info)
	  :m_world(world), m_viewer(world), m_resolution(resolution), m_view_pos(0, 0), m_show_debug_info(debug_info), m_is_exiting(0) {
		DASSERT(world);
		m_console = new hud::HudConsole(resolution);
		m_hud = new hud::Hud(world);
		m_target_info = new hud::HudTargetInfo(FRect(target_info_size));
		m_target_info->setVisible(false, false);

		m_game_mode = m_world->gameMode();
		ASSERT(m_game_mode);
		updatePC();

		if( const Actor *actor = getActor() )
			m_view_pos = int2(worldToScreen(actor->pos())) - resolution / 2;
		
		m_last_time = m_stats_update_time = getTime();
		m_last_look_at = float3(0, 0, 0);
		
		if(m_world->isClient())
			m_hud->showLayer(hud::layer_character);
	}

	Controller::~Controller() {
	}
		
	//TODO: better name
	void Controller::updatePC() {
		const GameClient *client = m_game_mode->currentClient();
		const vector<PlayableCharacter> &pcs = client? client->pcs : vector<PlayableCharacter>();
		PPlayableCharacter pc;

		if( (pcs.empty() && !m_pc) || (m_pc && pcs[0] == *m_pc) )
			return;

		m_pc = new PlayableCharacter(pcs[0]);
		m_pc_controller = m_pc? new PCController(*m_world, *m_pc) : nullptr;

		m_actor_ref = m_pc? m_pc->entityRef() : EntityRef();
		m_viewer.setSpectator(m_actor_ref);
		m_hud->setPCController(m_pc_controller);
	}
		
	Actor *Controller::getActor() {
		Actor *actor = nullptr;
		if(m_pc)
			actor = m_world->refEntity<Actor>(m_pc->entityRef());
		return actor;
	}
		
	void Controller::sendOrder(game::POrder &&order) {
		if(m_pc && m_world)
			m_world->sendOrder(std::move(order), m_pc->entityRef());
	}
		
	void Controller::onInput(const InputEvent &event) {
		if(m_console->handleInput(event) || m_hud->handleInput(event))
			return;

		bool mouse_over_hud = m_console->isMouseOver(event) || m_hud->isMouseOver(event);
		
		Actor *actor = getActor();
		if(actor && actor->isDead())
			actor = nullptr;

		if(event.keyDown('`'))
			m_console->setVisible(m_console->isVisible() ^ 1);
		else if(event.keyDown('H'))
			m_hud->setVisible(m_hud->isVisible() ^ 1);
		else if(event.keyDown('I'))
			m_hud->showLayer(hud::layer_inventory);
		else if(event.keyDown('C'))
			m_hud->showLayer(hud::layer_character);
		else if(event.keyDown('O'))
			m_hud->showLayer(hud::layer_options);
		else if(event.keyDown('V'))
			m_hud->showLayer(hud::layer_class);
		else if(event.keyDown('S'))
			m_hud->showLayer(hud::layer_stats);
		else if(event.keyDown('Q'))
			m_pc_controller->setStance(Stance::stand);
		else if(event.keyDown('A'))
			m_pc_controller->setStance(Stance::crouch);
		else if(event.keyDown('Z'))
			m_pc_controller->setStance(Stance::prone);
		else if(event.keyDown('R'))
			m_pc_controller->reload();

		if(event.mouseOver() && mouse_over_hud) {
			m_isect = m_full_isect = Intersection();
		}
		else if(event.mouseOver() && !mouse_over_hud) {
			m_screen_ray = screenRay((int2)event.mousePos() + m_view_pos);

			Flags::Type flags = Flags::walkable_tile | (Flags::entity & ~(Flags::projectile | Flags::impact | Flags::trigger));
			m_isect = m_viewer.pixelIntersect((int2)event.mousePos() + m_view_pos, {flags, m_actor_ref});
			if(m_isect.isEmpty() || m_isect.isTile())
				m_isect = m_viewer.trace(m_screen_ray, {flags, m_actor_ref});
	
			//TODO: pixel intersect may find an intersection, but the ray doesn't necessarily
			// has to intersect bounding box of the object
			m_full_isect = m_viewer.pixelIntersect((int2)event.mousePos() + m_view_pos, m_actor_ref);
			if(m_full_isect.isEmpty())
				m_full_isect = m_viewer.trace(m_screen_ray, m_actor_ref);

			if(!m_full_isect.isEmpty() && actor) {
				//TODO: send it only, when no other order is in progress (or has been sent and wasn't finished)
				if(m_full_isect.distance() < constant::inf && m_full_isect.distance() > -constant::inf) {
					float3 look_at = m_screen_ray.at(m_full_isect.distance());
					if(look_at != m_last_look_at && distance(look_at.xz(), actor->boundingBox().center().xz()) > 5.0f) {
						m_world->sendOrder(new LookAtOrder(look_at), m_actor_ref);
						m_last_look_at = look_at;
					}
				}
			}
		}
		else if(event.mouseKeyDown(0) && !event.hasModifier(InputEvent::mod_lctrl)) {
			Entity *entity = m_world->refEntity(m_isect);

			if(entity) {
				m_world->sendOrder(new InteractOrder(entity->ref()), m_actor_ref);
			}
			else if(m_isect.isTile()) {
				//TODO: pixel intersect always returns distance == 0
				int3 wpos = int3(m_screen_ray.at(m_isect.distance()) + float3(0, 0.5f, 0));
				
				bool run = actor && !isKeyPressed(Key::lshift);// && distance(float3(wpos), actor->pos()) > 10.0f;
				m_world->sendOrder(new MoveOrder(wpos, run), m_actor_ref);
			}
		}
		else if(event.mouseKeyDown(1) && actor) {
			AttackMode::Type mode = AttackMode::undefined;
			if(isKeyPressed(Key::lshift)) {
				const Weapon &weapon = actor->inventory().weapon();
				if(weapon.proto().attack_modes & AttackModeFlags::burst)
					mode = AttackMode::burst;
				else if(weapon.canKick())
					mode = AttackMode::kick;
			}

			if(!m_isect.isEmpty()) {
				Entity *entity = m_world->refEntity(m_isect);
				if(entity) {
					m_world->sendOrder(new AttackOrder(mode, entity->ref()), m_actor_ref);
				}
				else
					m_world->sendOrder(new AttackOrder(mode, m_screen_ray.at(m_isect.distance())), m_actor_ref);
			}
		}
		else if(event.keyDown('T') && !m_isect.isEmpty() && actor) {
			float3 pos = m_screen_ray.at(m_isect.distance());
			actor->setPos(int3(pos + float3(0.5f, 0.5f, 0.5f)));
			actor->fixPosition();
		}
	}

	void Controller::update(double time_diff) {
		if((isKeyPressed(Key::lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			m_view_pos -= getMouseMove();
		
		updatePC();

		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		if(actor)
			audio::setListener(actor->pos(), actor->estimateMove(1.0f), normalized(float3(-1, 0, -1)));
		else {
			Ray mid_ray = screenRay(m_view_pos + m_resolution / 2);
			auto isect = m_world->trace(mid_ray);
			if(isect) {
				float3 listener_pos = mid_ray.at(isect.distance());
				audio::setListener(listener_pos, float3(0, 0, 0), normalized(float3(-1, 0, -1)));
			}
		}

		if(actor && actor->isDead())
			actor = nullptr;

		if(!m_is_exiting) {
			vector<InputEvent> events = generateInputEvents();
			for(int n = 0; n < (int)events.size(); n++)
				onInput(events[n]);
			m_is_exiting = m_hud->exitRequested();
		}

		m_console->update(time_diff);
		m_hud->update(time_diff);
		m_target_info->update(time_diff);

		if(m_last_time - m_stats_update_time > 0.25) {
			m_profiler_stats = profiler::getStats();
			m_stats_update_time = m_last_time;
		}
		profiler::nextFrame();
		m_last_time = profiler::getTime();

		if( GameMode *game_mode = m_world->gameMode() ) {
			UserMessage message = game_mode->userMessage(UserMessageType::main);

			bool not_empty = !message.text.empty();
			hud::animateValue(m_main_message.anim_time, time_diff * 10.0f, not_empty);
			if(not_empty || m_main_message.anim_time < 0.01f)
				m_main_message.message = message;
		}

		{
			Actor *target_actor = m_world->refEntity<Actor>(m_isect);
			float2 info_size = m_target_info->rect().size();
			float2 pos = (float2)getMousePos() - float2(info_size.x * 0.5f, info_size.y + 30.0f);
			pos.x = clamp(pos.x, 0.0f, (float)m_resolution.x - info_size.x);
			if(pos.y < 0.0f)
				pos.y = getMousePos().y + 40.0f;
			m_target_info->setPos(pos);

			float hit_chance = 0.0f;		
			if(actor) {
				const Weapon &weapon = actor->inventory().weapon();
				if(!m_isect.isEmpty() && weapon.hasRangedAttack()) {
					FBox bbox = m_viewer.refBBox(m_isect);
					hit_chance = actor->estimateHitChance(actor->inventory().weapon(), bbox);
				}
			}

			const PlayableCharacter *pc = target_actor? m_game_mode->pc(m_game_mode->findPC(target_actor->ref())) : nullptr;
			const GameModeClient *gm_client = dynamic_cast<const GameModeClient*>(m_game_mode);
			if(gm_client && target_actor) {
				auto stats = gm_client->stats();
				for(const auto &stat : stats)
					if(stat.client_id == target_actor->clientId()) {
						m_target_info->setStats(stat);
						break;
					}
			}

			if(pc) {
				m_target_info->setCharacter(new Character(pc->character()));
				m_target_info->setHealth(target_actor->hitPoints() / target_actor->proto().actor->hit_points);
				m_target_info->setHitChance(hit_chance);
				m_target_info->setName(m_game_mode->client(target_actor->clientId())->nick_name);
			}
				
			m_target_info->setVisible(pc != nullptr);
		}
			

		while(true) {
			string command = m_console->getCommand();
			if(command.empty())
				break;

			printf("Invalid command: %s\n", command.c_str());
		}
	}

	void Controller::updateView(double time_diff) {
		m_viewer.update(time_diff);
	}

	void Controller::draw() const {
		clear(Color(0, 0, 0));
		SceneRenderer renderer(IRect(int2(0, 0), m_resolution), m_view_pos);

		m_viewer.addToRender(renderer);

		if(!m_isect.isEmpty())
			renderer.addBox(m_world->refBBox(m_isect), Color::yellow);

		/*const NaviMap *navi_map = m_world->naviMap(3);
		if(navi_map)
			navi_map->visualize(renderer, false);
		m_last_path.visualize(3, renderer);*/
		renderer.render();

		/*{ // Drawing cursor
			lookAt({0, 0});
			DTexture::unbind();
			drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
			drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));
		}*/

		if(m_show_debug_info)
			drawDebugInfo();

		lookAt({0, 0});

		m_hud->draw();
		m_target_info->draw();
		m_console->draw();

		if(!m_main_message.isEmpty()) {
			gfx::PFont font = gfx::Font::mgr["transformers_48"];
			FRect rect(float2(m_resolution.x, 30.0f));
			rect += float2(0.0f, m_console->rect().height());
			Color text_color = mulAlpha(Color::white, m_main_message.anim_time);
			Color shadow_color = mulAlpha(Color::black, m_main_message.anim_time);

			font->draw(rect, {text_color, shadow_color, HAlign::center}, m_main_message.text());
		}
	}
		
	void Controller::drawDebugInfo() const {
		TextFormatter fmt(4096);

		float3 isect_pos = m_screen_ray.at(m_isect.distance());
		fmt("View:(%d %d) Ray:(%.2f %.2f %.2f)\n", m_view_pos.x, m_view_pos.y, isect_pos.x, isect_pos.y, isect_pos.z);
		
		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		Actor *target_actor = m_world->refEntity<Actor>(m_isect);

		if(actor) {
			float3 actor_pos = actor->pos();
			fmt("Actor pos:(%.0f %.0f %.0f)\nHP: %d ", actor_pos.x, actor_pos.y, actor_pos.z, actor->hitPoints());
			if(target_actor)
				fmt("Target HP: %d", target_actor->hitPoints());

			const Weapon &weapon = actor->inventory().weapon();
			if(!m_isect.isEmpty() && weapon.hasRangedAttack()) {
				FBox bbox = m_viewer.refBBox(m_isect);
				float hit_chance = actor->estimateHitChance(actor->inventory().weapon(), bbox);
				fmt("\nHit chance: %.0f%%", hit_chance * 100.0f);
			}
			fmt("\n\n");
		}

		fmt("%s", m_profiler_stats.c_str());

		lookAt({0, 0});
		gfx::PFont font = gfx::Font::mgr["liberation_16"];
	
		int2 extents = font->evalExtents(fmt.text()).size();
		extents.y = (extents.y + 19) / 20 * 20;
		int2 res = getWindowSize();
		int2 pos(res.x - extents.x - 4, res.y - extents.y - 4);
		drawQuad(pos.x, pos.y, res.x, res.y, Color(0, 0, 0, 80));
		font->draw(pos + int2(2, 2), {Color::white, Color::black}, fmt);
	}

}
