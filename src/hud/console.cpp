/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/console.h"
#include "hud/edit_box.h"
#include "io/input.h"

namespace hud {

	enum {
		console_height = 22,
		max_command_length = 128,
	};

	HudConsole::HudConsole(const int2 &resolution) :HudLayer(FRect(0, 0, resolution.x, console_height), HudLayer::slide_top) {
		m_edit_box = new HudEditBox(rect(), max_command_length);
		attach(m_edit_box.get());
		m_edit_box->setStyle(getStyle(HudStyleId::console));
		setVisible(false, false);
	}

	HudConsole::~HudConsole() { }

	bool HudConsole::onInput(const io::InputEvent &event) {
		if(event.keyDown('`') || (event.mouseKeyDown(0) && !isMouseOver(event))) {
			setVisible(false);
			return true;
		}

		return false;
	}

	bool HudConsole::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::text_modified && event.source == m_edit_box) {
			m_commands.push_back(m_edit_box->text());
			m_edit_box->setText("");
			return true;
		}

		return false;
	}

	void HudConsole::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);

		m_edit_box->setInputFocus(m_is_visible);
		if(!m_is_visible)
			m_edit_box->setText("");
	}

	void HudConsole::onDraw() const {
//		HudLayer::onDraw();
	}

	const string HudConsole::getCommand() {
		string out;
		if(!m_commands.empty()) {
			out = m_commands.front();
			m_commands.erase(m_commands.begin());
		}
		return out;
	}

}
