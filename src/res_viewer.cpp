/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/drawing.h"

#include "game/sprite.h"
#include "game/tile.h"

#include "ui/list_box.h"
#include "ui/button.h"
#include "ui/message_box.h"
#include "sys/config.h"

using namespace game;
using namespace ui;

enum class ResType {
	unknown,
	tile,
	sprite,
	texture,
};

ResType classifyFileName(const string &file_name) {
	auto locase_name = toLower(file_name);

	if(removeSuffix(locase_name, ".zar") || removeSuffix(locase_name, ".png"))
		return ResType::texture;
	else if(removeSuffix(locase_name, ".tile"))
		return ResType::tile;
	else if(removeSuffix(locase_name, ".sprite"))
		return ResType::sprite;

	return ResType::unknown;
}

class Resource {
  public:
	Resource(const FilePath &current_dir, const string &file_name)
		: m_file_name(file_name), m_type(classifyFileName(file_name)) {
		DASSERT(m_type != ResType::unknown);
		m_font = res::getFont(ui::WindowStyle::fonts[0]);

		Loader loader(current_dir / file_name);
		if(m_type == ResType::tile) {
			m_tile = make_immutable<Tile>("", loader);
			m_rect_size = m_tile->rect().size() + int2(8, 8);
		} else if(m_type == ResType::texture) {
			m_texture = make_immutable<DTexture>("", loader);
			m_rect_size = m_texture->size();
		} else if(m_type == ResType::sprite) {
			//	printf("Loading sprite: %s\n", file_name);
			m_sprite = make_shared<Sprite>();
			m_sprite->setResourceName(file_name);
			loader >> *m_sprite;
			m_sprite->printInfo();
			m_rect_size =
				worldToScreen(IBox({-4, -4, -4}, m_sprite->bboxSize() + int3{4, 4, 4})).size();
			m_last_time = getTime();
			m_frame_id = m_dir_id = m_seq_id = 0;
			updateFrameId();
		}
	}

	void printStats(Renderer2D &out, int2 pos) const {
		TextFormatter fmt;

		if(m_type == ResType::sprite) {
			bool is_gui_image = (*m_sprite)[m_seq_id].name.find("gui") != string::npos;
			auto &seq = (*m_sprite)[m_seq_id];

			int2 max_frame_size(0, 0);
			for(int n = 0; n < seq.frame_count; n++) {
				if(m_sprite->frame(m_seq_id, n).id < 0)
					continue;
				max_frame_size =
					max(max_frame_size, m_sprite->getRect(m_seq_id, n, m_dir_id).size());
			}

			fmt("Sequence: % / %\n%\nFrames: %\nBounding box: "
				"(%)\nMax frame size: (%)",
				m_seq_id, (int)m_sprite->size(), seq.name.c_str(), seq.frame_count,
				m_sprite->bboxSize(), max_frame_size);

			m_font->draw(out, (float2)pos, {ColorId::white, ColorId::black}, fmt.text());
			pos.y += m_font->evalExtents(fmt.text()).height();

			double time = getTime();
			for(int n = 0; n < (int)m_events.size(); n++) {
				FColor col((float)(m_events[n].second - time + 1.0), 0.0f, 0.0f);
				m_font->draw(out, (float2)pos, {col, ColorId::black}, m_events[n].first);
				pos.y += m_font->lineHeight();
			}

		} else if(m_type == ResType::texture) {
			fmt("Size: (%)", m_texture->size());
			m_font->draw(out, (float2)pos, {ColorId::white, ColorId::black}, fmt.text());
		}
	}

	void updateFrameId() {
		if(m_type != ResType::sprite)
			return;

		int id;
		while((id = m_sprite->frame(m_seq_id, m_frame_id).id) < 0) {
			const char *event_name = Sprite::eventIdToString((Sprite::EventId)id);
			if(id == Sprite::ev_overlay)
				event_name = "overlay";
			if(event_name)
				m_events.push_back(make_pair(event_name, getTime()));

			m_frame_id++;
			if(m_frame_id == m_sprite->frameCount(m_seq_id))
				m_frame_id = 0;
		}
	}

	void update(double time) {
		if(m_type == ResType::sprite) {
			if(time - m_last_time > 1 / 15.0) {
				m_frame_id = m_frame_id + 1;
				m_frame_id %= m_sprite->frameCount(m_seq_id);
				m_last_time = time;
			}

			updateFrameId();

			vector<pair<const char *, double>> tevents;
			for(int n = 0; n < (int)m_events.size(); n++)
				if(m_events[n].second > time - 1.0)
					tevents.push_back(m_events[n]);
			m_events.swap(tevents);
		}
	}

	void onInput(const InputState &state) {
		if(m_type == ResType::texture) {
			if(state.isKeyDown('E')) {
				string name = FilePath(m_file_name).relative();
				THROW("fixme");
				removeSuffix(name, ".zar");
				name += ".tga";
				printf("Exporting: %s\n", name.c_str());
				Texture tex;
				m_texture->download(tex);
				Saver svr(name);
				tex.save(svr);
			}
		}
		if(m_type == ResType::sprite) {
			if(state.isKeyDown(InputKey::up))
				m_seq_id++;
			if(state.isKeyDown(InputKey::down))
				m_seq_id--;
			if(state.isKeyDown(InputKey::left))
				m_dir_id--;
			if(state.isKeyDown(InputKey::right))
				m_dir_id++;
			if(state.isKeyDown('P')) {
				FilePath path(m_file_name);
				printf("Sequences for: %s\n", path.relative().c_str());
				for(int s = 0; s < m_sprite->size(); s++)
					printf("Seq %3d: %s\n", s, (*m_sprite)[s].name.c_str());
				printf("\n");
			}

			m_seq_id = (m_seq_id + (int)m_sprite->size()) % (int)m_sprite->size();
			int dir_count = m_sprite->dirCount(m_seq_id);
			m_dir_id = (m_dir_id + dir_count) % dir_count;
			m_frame_id %= m_sprite->frameCount(m_seq_id);
			updateFrameId();
		}
	}

	void draw(Renderer2D &out, int2 pos, bool is_selected) const {
		Color outline_col = is_selected ? ColorId::red : Color(255, 255, 255, 100);

		if(m_type == ResType::tile) {
			out.setViewPos(-pos + m_tile->rect().min());
			IBox box(int3(0, 0, 0), m_tile->bboxSize());
			m_tile->draw(out, int2(0, 0));
			drawBBox(out, box, outline_col);
		} else if(m_type == ResType::texture) {
			out.setViewPos(-pos);
			out.addFilledRect(IRect(m_rect_size), m_texture);
			out.addRect(IRect({0, 0}, m_rect_size), outline_col);
		} else if(m_type == ResType::sprite) {
			bool is_gui_image = (*m_sprite)[m_seq_id].name.find("gui") != string::npos;

			DASSERT(m_sprite->frame(m_seq_id, m_frame_id).id >= 0);
			IRect rect = m_sprite->getRect(m_seq_id, m_frame_id, m_dir_id);
			FRect tex_rect;
			auto dtex = m_sprite->getFrame(m_seq_id, m_frame_id, m_dir_id, tex_rect);

			IBox box({0, 0, 0}, m_sprite->bboxSize());
			IRect brect = worldToScreen(IBox(box.min() - int3(4, 4, 4), box.max() + int3(4, 4, 4)));
			if(is_gui_image) {
				rect -= rect.min();
				brect -= brect.min();
			}
			out.setViewPos(brect.min() - pos);
			out.addFilledRect(FRect(rect), tex_rect, dtex);

			if(is_gui_image)
				out.addRect(rect, outline_col);
			else
				drawBBox(out, box, outline_col);
		}
	}

	ResType type() const { return m_type; }
	int2 rectSize() const { return m_rect_size; }
	const string &fileName() const { return m_file_name; }

  private:
	string m_file_name;
	ResType m_type;
	unique_ptr<Font> m_font;

	int2 m_rect_size;
	PTile m_tile;
	PTexture m_texture;
	PSprite m_sprite;

	vector<pair<const char *, double>> m_events;
	double m_last_time;
	int m_frame_id, m_seq_id, m_dir_id;
};

class ResourceView : public Window {
  public:
	virtual const char *className() const { return "ResourceView"; }
	ResourceView(IRect rect, FilePath current_dir, vector<string> file_names)
		: Window(rect), m_selected_id(-1) {
		for(auto file_name : file_names) {
			auto res_type = classifyFileName(file_name);
			if(res_type != ResType::unknown) {
				try {
					auto new_res = make_unique<Resource>(current_dir, file_name);
					m_resources.emplace_back(std::move(new_res));
				} catch(const Exception &ex) {
					printf("When loading %s:\n%s\n", file_name.c_str(), ex.what());
				}
			}
		}

		int spacing = 4, width = clippedRect().width(), cur_height = 0;
		int2 pos(spacing, spacing + 100);

		for(int n = 0; n < (int)m_resources.size(); n++) {
			auto &res = m_resources[n];
			m_positions.emplace_back(pos);
			cur_height = max(cur_height, res->rectSize().y);

			if(n + 1 < (int)m_resources.size()) {
				pos.x += res->rectSize().x + spacing;
				if(m_resources[n + 1]->rectSize().x + pos.x > width) {
					pos.x = spacing;
					pos.y += cur_height + spacing;
					cur_height = 0;
				}
			}
		}

		setInnerRect(IRect(int2(width, pos.y + cur_height)));
	}

	void select(const string &file_name) {
		for(int n = 0; n < (int)m_resources.size(); n++)
			if(m_resources[n]->fileName() == file_name) {
				if(m_selected_id != n)
					setInnerOffset(int2(0, m_positions[n].y - 5));
				m_selected_id = n;
			}
	}

	void update() {
		double time = getTime();
		for(auto &resource : m_resources)
			resource->update(time);
	}

	void onInput(const InputState &state) override {
		m_last_mouse_pos = state.mousePos();
		int2 offset = innerOffset() - clippedRect().min();

		for(auto &resource : m_resources)
			resource->onInput(state);

		bool clicked = state.isMouseButtonPressed(InputButton::left);

		if(clicked)
			m_selected_id = -1;
		for(int n = 0; n < (int)m_resources.size(); n++) {
			const auto &res = m_resources[n];
			int2 pos = m_positions[n] - offset;
			if(clicked && IRect(pos, pos + res->rectSize()).containsPixel(m_last_mouse_pos))
				m_selected_id = n;
		}
		if(clicked)
			sendEvent(this, Event::element_selected, m_selected_id);
	}

	void drawContents(Renderer2D &out) const override {
		int2 offset = innerOffset() - clippedRect().min();

		for(int n = 0; n < (int)m_resources.size(); n++) {
			const auto &res = m_resources[n];

			auto pos = m_positions[n] - offset;
			if(pos.y + res->rectSize().y < clippedRect().y())
				continue;
			if(pos.y > clippedRect().ey())
				break;

			res->draw(out, m_positions[n] - offset, n == m_selected_id);
		}

		if(m_selected_id != -1) {
			out.setViewPos(-clippedRect().min());
			m_resources[m_selected_id]->printStats(out, int2(0, 0));
		}
	}

	const auto &operator[](int res_id) const {
		DASSERT(res_id >= 0 && res_id < (int)m_resources.size());
		return *m_resources[res_id];
	}

  private:
	int m_selected_id;
	int2 m_last_mouse_pos;
	vector<unique_ptr<Resource>> m_resources;
	vector<int2> m_positions;
};

enum class Command { empty, change_dir, exit };

class ResViewerWindow : public Window {
  public:
	ResViewerWindow(int2 res, const string &path)
		: Window(IRect(res), WindowStyle::gui_light), m_current_dir(FilePath(path).absolute()) {
		m_command = make_pair(Command::empty, string());

		int left_width = 300;
		m_dir_view = make_shared<ListBox>(IRect(0, 0, left_width, res.y));

		m_entries = findFiles(m_current_dir, FindFiles::regular_file | FindFiles::directory |
												 FindFiles::relative | FindFiles::include_parent);
		std::sort(m_entries.begin(), m_entries.end());
		vector<string> names;

		for(auto entry : m_entries) {
			m_dir_view->addEntry(entry.path.c_str(), entry.is_dir ? ColorId::yellow : ColorId::white);
			if(!entry.is_dir)
				names.emplace_back(entry.path);
		}

		m_res_view =
			make_shared<ResourceView>(IRect(left_width + 2, 0, res.x, res.y), m_current_dir, names);

		attach(m_dir_view);
		attach(m_res_view);
	}

	void resize(const int2 &new_size) {
		setRect(IRect(new_size));
		// TODO: resize controls
	}

	pair<Command, string> command() const { return m_command; }
	string currentDir() const { return m_current_dir; }

	void process(const InputState &state) override {
		m_res_view->update();
		Window::process(state);
	}

	bool onEvent(const Event &ev) override {
		if(ev.type == Event::window_closed && ev.source == popup) {
			popup = nullptr;
			if(ev.value == 1)
				m_command = make_pair(Command::exit, string());
		} else if(ev.type == Event::element_selected) {
			if(m_dir_view.get() == ev.source && ev.value >= 0 && ev.value < (int)m_entries.size()) {
				const FileEntry &entry = m_entries[ev.value];

				if(entry.is_dir) {
					m_command = make_pair(Command::change_dir, m_current_dir / entry.path);
				} else {
					m_res_view->select((*m_dir_view)[ev.value].text);
				}
			} else if(m_res_view.get() == ev.source) {
				int id = ev.value;
				if(id != -1) {
					auto &res = (*m_res_view)[id];
					for(int n = 0; n < m_dir_view->size(); n++)
						if((*m_dir_view)[n].text == res.fileName())
							id = n;
				}
				m_dir_view->selectEntry(id);
			}
		} else if(ev.type == Event::escape) {
			if(!popup) {
				IRect popup_rect = IRect(-150, -40, 150, 40) + center();
				popup = make_shared<MessageBox>(popup_rect, "Do you want to quit?",
												MessageBoxMode::yes_no);
				attach(popup, true);
			}
		} else
			return false;

		return true;
	}

  private:
	vector<FileEntry> m_entries;
	FilePath m_current_dir;
	pair<Command, string> m_command;

	PListBox m_dir_view;
	shared_ptr<ResourceView> m_res_view;

	PWindow popup;
};

static unique_ptr<ResViewerWindow> main_window;
static double start_time = getTime();

static bool main_loop(GfxDevice &device, void*) {
	DASSERT(main_window);

	Tile::setFrameCounter((int)((getTime() - start_time) * 15.0));
	TextureCache::main_cache.nextFrame();

	GfxDevice::clearColor(Color(0, 0, 0));
	Renderer2D out(IRect(device.windowSize()));

	main_window->process(device.inputState());
	auto command = main_window->command();
	if(command.first == Command::exit)
		return false;
	if(command.first == Command::change_dir)
		main_window = make_unique<ResViewerWindow>(device.windowSize(), command.second);
	if(main_window->size() != device.windowSize())
		main_window->resize(device.windowSize());

	main_window->draw(out);
	out.render();

	return true;
}

int safe_main(int argc, char **argv) {
	Config config("res_viewer");

	Profiler profiler;
	GfxDevice gfx_device;
	createWindow("res_viewer", gfx_device, config.resolution, config.window_pos, config.fullscreen_on);

	main_window = make_unique<ResViewerWindow>(gfx_device.windowSize(), "data/");
	gfx_device.runMainLoop(main_loop);

	main_window.reset();

	return 0;
}

int main(int argc, char **argv) {
	try {
		std::setlocale(LC_ALL, "en_US.UTF8");
		std::setlocale(LC_NUMERIC, "C");
		return safe_main(argc, argv);
	} catch(const Exception &ex) {
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), ex.backtrace().c_str());
		return 1;
	}
}
