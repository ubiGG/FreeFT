#ifndef GFX_TEXTURE_H
#define GFX_TEXTURE_H

#include "base.h"

namespace gfx
{

	enum TextureIdent
	{
		TI_Unknown       =  0,

		TI_R8G8B8        = 20,
		TI_A8R8G8B8      = 21,
		TI_X8R8G8B8      = 22,
		TI_R5G6B5        = 23,
		TI_X1R5G5B5      = 24,
		TI_A1R5G5B5      = 25,
		TI_A4R4G4B4      = 26,
		TI_R3G3B2        = 27,
		TI_A8            = 28,
		TI_A8R3G3B2      = 29,
		TI_X4R4G4B4      = 30,
		TI_A2B10G10R10   = 31,
		TI_A8B8G8R8      = 32,
		TI_X8B8G8R8      = 33,
		TI_G16R16        = 34,
		TI_A2R10G10B10   = 35,
		TI_A16B16G16R16  = 36,

		TI_L8            = 50,
		TI_A8L8          = 51,
		TI_A4L4          = 52,

		TI_V8U8          = 60,
		TI_L6V5U5        = 61,
		TI_X8L8V8U8      = 62,
		TI_Q8W8V8U8      = 63,
		TI_V16U16        = 64,
		TI_A2W10V10U10   = 67,

		TI_UYVY          = 0x59565955,  //MAKEFOURCC('U', 'Y', 'V', 'Y'),
		TI_R8G8_B8G8     = 0x47424752,  //MAKEFOURCC('R', 'G', 'B', 'G'),
		TI_YUY2          = 0x32595559,  //MAKEFOURCC('Y', 'U', 'Y', '2'),
		TI_G8R8_G8B8     = 0x42475247,  //MAKEFOURCC('G', 'R', 'G', 'B'),
		TI_DXT1          = 0x31545844,  //MAKEFOURCC('D', 'X', 'T', '1'),
		TI_DXT2          = 0x32545844,  //MAKEFOURCC('D', 'X', 'T', '2'),
		TI_DXT3          = 0x33545844,  //MAKEFOURCC('D', 'X', 'T', '3'),
		TI_DXT4          = 0x34545844,  //MAKEFOURCC('D', 'X', 'T', '4'),
		TI_DXT5          = 0x35545844,  //MAKEFOURCC('D', 'X', 'T', '5'),

		TI_L16           = 81,
		TI_Q16W16V16U16  =110,

		// Floating point surface formats
		// s10e5 formats (16-bits per channel)
		TI_R16F          = 111,
		TI_G16R16F       = 112,
		TI_A16B16G16R16F = 113,

		// IEEE s23e8 formats (32-bits per channel)
		TI_R32F          = 114,
		TI_G32R32F       = 115,
		TI_A32B32G32R32F = 116,

		TI_FORCE_DWORD   =0x7fffffff
	};

	class TextureFormat {
	public:
		TextureFormat(int internal, int format, int type, bool compressed = 0);
		TextureFormat(int internal);
		TextureFormat(TextureIdent fmt);
		TextureFormat();

		TextureIdent ident() const;
		int glInternal() const;
		int glFormat() const;
		int glType() const;
		bool isCompressed() const;

		int bytesPerPixel() const;

		int evalImageSize(int width, int height) const;
		int evalLineSize(int width) const;

		bool operator==(const TextureFormat&) const;
		bool isSupported() const;

	private:
		unsigned id;
	};

	struct Bitmap {
		void clear() {
			width = height = 0;
			bits.clear();
		}

		int width, height;
		vector<u8> bits;
	};

	class Texture;
	class PalTexture;

	class CompressedTexture {
	public:
		CompressedTexture();

		void serialize(Serializer&);
		void legacyLoad(Serializer&, bool fast_compression);

		void decompress(PalTexture&) const;

		bool operator==(const CompressedTexture&) const;

		int width() const { return m_width; }
		int height() const { return m_height; }
		const int2 dimensions() const { return int2(m_width, m_height); }
		int memorySize() const { return (int)(sizeof(CompressedTexture) + m_stream.size()); }

	protected:
		friend class PalTexture;

		int m_width, m_height;
		int m_palette_size;
		int m_dec_stream_size;
		vector<char> m_stream;
	};

	class PalTexture {
	public:
		PalTexture(int width = 0, int height = 0);
		
		void legacyLoad(Serializer&);
		void serialize(Serializer&);
		void compress(CompressedTexture&, bool fast) const;
		
		int width() const { return m_width; }
		int height() const { return m_height; }
		const int2 dimensions() const { return int2(m_width, m_height); }

		void toBitmap(Bitmap&) const;
		void toTexture(Texture&, const vector<Color> *palette = nullptr) const;

//	protected:
		friend class CompressedTexture;

		vector<Color> m_palette;
		vector<u8> m_colors; //TODO: merge colors & alpha into one vector
		vector<u8> m_alphas;
		int m_width, m_height;
	};

	class Texture: public RefCounter
	{
	public:
		Texture(int width, int height);
		Texture(Texture&&);
		Texture(const Texture&) = default;
		Texture();

		//TODO: make non-destructive
		void resize(int width, int height);
		void free();

		int width() const { return m_width; }
		int height() const { return m_height; }
		int2 size() const { return int2(m_width, m_height); }
		bool isEmpty() const { return data.empty(); }
		bool testPixel(const int2&) const;

		TextureFormat format() const { return TextureFormat(TI_A8B8G8R8); }

		// Loading from TGA, BMP, PNG, DDS
		void serialize(Serializer&);
		void swap(Texture&);

		Color* line(int y) { DASSERT(y < m_height); return &data[y * m_width]; }
		const Color* line(int y) const { DASSERT(y < m_height); return &data[y * m_width]; }

		Color& operator()(int x, int y) { return data[x + y * m_width]; }
		const Color& operator()(int x, int y) const { return data[x + y * m_width]; }
		
		void loadTGA(Serializer&);
		void loadBMP(Serializer&);
		void loadPNG(Serializer&);
		void loadZAR(Serializer&);

		void saveTGA(Serializer&);

private:
		vector<Color> data;
		int m_width, m_height;
	};
}
#endif
