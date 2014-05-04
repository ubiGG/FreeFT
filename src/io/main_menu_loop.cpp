/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "ui/file_dialog.h"
#include "ui/image_button.h"
#include "io/main_menu_loop.h"
#include "io/single_player_loop.h"
#include "io/multi_player_loop.h"
#include "io/server_loop.h"
#include "gfx/device.h"
#include "gfx/opengl.h"
#include "audio/device.h"

namespace io {

	using namespace gfx;
	using namespace ui;

	static PImageButton makeButton(const int2 &pos, const char *title) {
		char back_name[256];
		snprintf(back_name, sizeof(back_name), "btn/big/%d", rand() % 10 + 1);

		ImageButtonProto proto(back_name, "btn/big/Up", "btn/big/Dn", "transformers_30", FRect(0.25f, 0.05f, 0.95f, 0.95f));
		proto.sound_name = "butn_bigred";
		return new ImageButton(pos, proto, title, ImageButton::mode_normal);
	}

	MainMenuLoop::MainMenuLoop() :Window(IRect({0, 0}, gfx::getWindowSize()), Color::transparent), m_mode(mode_normal) {
		m_back = gfx::DTexture::gui_mgr["back/flaminghelmet"];
		m_loading = gfx::DTexture::gui_mgr["misc/worldm/OLD_moving"];

		m_anim_pos = 0.0;
		m_blend_time = 1.0;
		m_timer = 0.0;

		IRect rect = localRect();
		m_back_rect = (IRect)FRect(
			float2(rect.center()) - float2(m_back->dimensions()) * 0.5f,
			float2(rect.center()) + float2(m_back->dimensions()) * 0.5f);

		m_single_player	= makeButton(m_back_rect.min + int2(500, 70), "Single player");
		m_multi_player	= makeButton(m_back_rect.min + int2(500, 115), "Multi player");
		m_create_server	= makeButton(m_back_rect.min + int2(500, 160), "Create server");
		m_options		= makeButton(m_back_rect.min + int2(500, 205), "Options");
		m_credits		= makeButton(m_back_rect.min + int2(500, 250), "Credits");
		m_exit			= makeButton(m_back_rect.min + int2(500, 295), "Exit");

		attach(m_single_player.get());
		attach(m_multi_player.get());
		attach(m_create_server.get());
		attach(m_options.get());
		attach(m_credits.get());
		attach(m_exit.get());

		startMusic();
	}
		
	void MainMenuLoop::stopMusic() {
		if(m_music) {
			m_music->stop(m_blend_time);
			m_music.reset();
		}
	}

	void MainMenuLoop::startMusic() {
		if(m_music && m_music->isPlaying())
			return;

		const char *music_files[] = {
			"data/music/gui/MX_ENV_MENU_MAIN1.mp3",
			"data/music/gui/MX_ENV_MENU_MAIN2.mp3",
			"data/music/gui/MX_MENU_WORLDMAP1.mp3",
			"data/music/gui/MX_MENU_WORLDMAP2.mp3"
		};

		m_music	= audio::playMusic(music_files[rand() % COUNTOF(music_files)], 1.0f);
		m_start_music_time = -1.0;
	}
		
	bool MainMenuLoop::onEvent(const Event &ev) {
		if(m_mode == mode_normal && ev.type == Event::button_clicked && ev.source->parent() == this) {
			if(ev.source == m_single_player.get()) {
				m_mode = mode_starting_single;
				IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
				m_file_dialog = new FileDialog(dialog_rect, "Select map", FileDialogMode::opening_file);
				m_file_dialog->setPath("data/maps/");
				attach(m_file_dialog.get(), true);
			}
			else if(ev.source == m_create_server.get()) {
				m_mode = mode_starting_server;
				IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
				m_file_dialog = new FileDialog(dialog_rect, "Select map", FileDialogMode::opening_file);
				m_file_dialog->setPath("data/maps/");
				attach(m_file_dialog.get(), true);
			}
			else if(ev.source == m_multi_player.get()) {
				m_sub_loop.reset(new MultiPlayerLoop("", 20001));
			}
			else if(ev.source == m_exit.get()) {
				stopMusic();
				m_mode = mode_quitting;
				m_timer = 1.0;
			}

			return true;
		}
		else if(ev.type == Event::window_closed && m_file_dialog.get() == ev.source) {
			string path = m_file_dialog->path();

			if(m_mode == mode_starting_single && ev.value) {
				m_future_loop = std::async(std::launch::async, [path]() { return PLoop(new SinglePlayerLoop(path)); } );
			}
			else if(m_mode == mode_starting_server && ev.value) {
				m_future_loop = std::async(std::launch::async, [path]() { return PLoop(new ServerLoop(path)); } );
			}
			else if(m_mode == mode_starting_multi && ev.value) {
				m_future_loop = std::async(std::launch::async, []() { return PLoop(new MultiPlayerLoop("", 20001)); } );
			}
			
			m_mode = mode_normal;
			stopMusic();
		}

		return false;
	}

	void MainMenuLoop::drawLoading(const int2 &pos, float alpha) const {
		const char *text = "Loading";
		gfx::PFont font = gfx::Font::mgr["transformers_30"];
		Color color(1.0f, 0.8f, 0.2f, alpha);

		lookAt(-pos);

		int2 dims(m_loading->dimensions());
		float2 center = float2(dims.x * 0.49f, dims.y * 0.49f);

		float scale = 1.0f + pow(sin(m_anim_pos * 0.5 * constant::pi * 2.0), 8.0) * 0.1;

		IRect extents = font->evalExtents(text);
		font->drawShadowed(int2(0, -extents.height()), color, Color::black, text);

		glPushMatrix();
		glTranslatef(extents.width() + 8.0f + center.x, 0.0f, 0.0f);

		glScalef(scale, scale, scale);
		glRotated(m_anim_pos * 360.0, 0, 0, 1);
		glTranslatef(-center.x, -center.y, 0);

		m_loading->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		drawQuad(int2(0, 0), dims, color);
		glPopMatrix();
	}

	bool MainMenuLoop::tick(double time_diff) {
		using namespace gfx;

		if(m_sub_loop) {
			if(!m_sub_loop->tick(time_diff))
				m_sub_loop.reset(nullptr);
			return true;
		}

		if(m_future_loop.valid()) {
			if(m_future_loop.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				m_sub_loop = std::move(m_future_loop.get());
			}
		}


		if((!m_music || !m_music->isPlaying()) && m_start_music_time < 0.0)
			m_start_music_time = 5.0;

		if(m_start_music_time > 0.0) {
			m_start_music_time -= time_diff;
			if(m_start_music_time <= 0.0)
				startMusic();
		}

		if(m_mode != mode_quitting)
			process();

		clear(Color(0, 0, 0));
		m_back->bind();
		drawQuad(m_back_rect.min, m_back_rect.size());
	
		lookAt({0, 0});
		draw();

		if(m_future_loop.valid())
			drawLoading(gfx::getWindowSize() - int2(180, 50), 1.0);

		lookAt({0, 0});
		if(m_mode == mode_quitting) {
			DTexture::bind0();
			m_timer -= time_diff / m_blend_time;
			if(m_timer < 0.0) {
				m_timer = 0.0;
				m_mode = mode_quit;
			}
			drawQuad({0, 0}, gfx::getWindowSize(), Color(1.0f, 1.0f, 1.0f, 1.0f - m_timer));
		}

		m_anim_pos += time_diff;
		if(m_anim_pos > 1.0)
			m_anim_pos -= 1.0;

		return m_mode != mode_quit;
	}

}
