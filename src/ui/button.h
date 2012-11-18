#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui/window.h"
#include "gfx/font.h"

namespace ui
{

	class Button: public Window
	{
	public:
		Button(IRect rect, const char *text, int id = 0);
		virtual const char *typeName() const { return "Button"; }

		virtual void drawContents() const;
		virtual void onInput(int2 mouse_pos);
		virtual void setText(const char *text);

		void enable(bool);
		int id() const { return m_id; }

	protected:
		int m_id;
		bool m_is_enabled;
		bool m_mouse_press;
		IRect m_text_extents;
		string m_text;
		gfx::PFont m_font;
	};

	typedef Ptr<Button> PButton;


}


#endif
