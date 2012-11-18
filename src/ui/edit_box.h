#ifndef UI_EDIT_BOX_H
#define UI_EDIT_BOX_H

#include "ui/window.h"
#include "gfx/font.h"


namespace ui {

	class EditBox: public Window
	{
	public:
		EditBox(const IRect &rect, int max_size, Color col = Color::transparent);
		virtual const char *typeName() const { return "EditBox"; }

		void setText(const char *text);
		const char *text() const { return m_text.c_str(); }

		void drawContents() const;
		void onInput(int2);
		bool onEvent(const Event&);

	private:
		void onKey(int key);
		void setCursorPos(int2);

		int m_max_size;
		int m_last_key;
		double m_key_down_time;
		double m_on_key_time;

		gfx::PFont m_font;
		string m_text, m_old_text;
		int m_cursor_pos;
		bool m_is_editing;
	};

	typedef Ptr<EditBox> PEditBox;

}

#endif

