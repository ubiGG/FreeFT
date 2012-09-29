#include "gfx/device.h"
#include <GL/gl.h>

namespace gfx
{

	static float defaultMtx[16];

	void InitViewport(int2 size) {
		glViewport(0, 0, size.x, size.y);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, size.x, 0, size.y, -1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0.0f, size.y, 0.0f);
		glScalef(1.0f, -1.0f, 1.0f);

		glGetFloatv(GL_MODELVIEW_MATRIX, defaultMtx);
	}

	void LookAt(int2 pos) {
		glLoadMatrixf(defaultMtx);
		glTranslatef(-pos.x, -pos.y, 0.0f);
	}

	void DrawQuad(int2 pos, int2 size, Color color) {
		glBegin(GL_QUADS);

		glColor4ub(color.r, color.g, color.b, color.a);
		glTexCoord2f(0.0f, 0.0f);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(1.0f, 0.0f);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(1.0f, 1.0f);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(0.0f, 1.0f);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}

	void DrawQuad(int2 pos, int2 size, float2 uv0, float2 uv1, Color color) {
		glBegin(GL_QUADS);

		glColor4ub(color.r, color.g, color.b, color.a);
		glTexCoord2f(uv0.x, uv0.y);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(uv1.x, uv0.y);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(uv1.x, uv1.y);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(uv0.x, uv1.y);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}

	void DrawLine(int3 wp1, int3 wp2, Color color) {
		glBegin(GL_LINES);

		float2 p1 = WorldToScreen(wp1);
		float2 p2 = WorldToScreen(wp2);

		glColor4ub(color.r, color.g, color.b, color.a);
		glVertex2f(p1.x, p1.y);
		glVertex2f(p2.x, p2.y);

		glEnd();
	}

	void DrawRect(const IRect &rect, Color col) {
		glBegin(GL_LINE_STRIP);
		glColor4ub(col.r, col.g, col.b, col.a);

		glVertex2i(rect.min.x, rect.min.y);
		glVertex2i(rect.max.x, rect.min.y);
		glVertex2i(rect.max.x, rect.max.y);
		glVertex2i(rect.min.x, rect.max.y);
		glVertex2i(rect.min.x, rect.min.y);

		glEnd();
	}

	void DrawBBox(const IBox &box, Color col) {
		float2 vx = WorldToScreen(int3(box.Width(), 0, 0));
		float2 vy = WorldToScreen(int3(0, box.Height(), 0));
		float2 vz = WorldToScreen(int3(0, 0, box.Depth()));
		float2 pos = WorldToScreen(box.min);

		float2 pt[8] = {
			pos + vx + vy,
			pos + vx + vy + vz,
			pos + vz + vy,
			pos + vy,
			pos + vx,
			pos + vx + vz,
			pos + vz,
			pos,
		};

		int back[] = {6, 7, 3, 7, 4};
		int front[] = {5, 4, 0, 1, 2, 6, 5, 1, 0, 3, 2};

		glBegin(GL_LINE_STRIP);
		glColor4ub(col.r >> 1, col.g >> 1, col.b >> 1, col.a);
		for(size_t n = 0; n < COUNTOF(back); n++)
			glVertex2f(pt[back[n]].x, pt[back[n]].y);

		glColor4ub(col.r, col.g, col.b, col.a);
		for(size_t n = 0; n < COUNTOF(front); n++)
			glVertex2f(pt[front[n]].x, pt[front[n]].y);
		glEnd();
	}

	void Clear(Color color) {
		float4 col = color;
		glClearColor(col.x, col.y, col.z, col.w);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void SetBlendingMode(BlendingMode mode) {
		if(mode == bmDisabled)
			glDisable(GL_BLEND);
		else
			glEnable(GL_BLEND);

		if(mode == bmNormal)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

}
