/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_TEXTURE_CACHE_H
#define GFX_TEXTURE_CACHE_H

#include "base.h"

class TextureCache;

class CachedTexture {
  public:
	CachedTexture();
	CachedTexture(const CachedTexture &);
	void operator=(const CachedTexture &);
	virtual ~CachedTexture();

	STexture accessTexture(FRect &, bool put_in_atlas = true) const;
	TextureCache *getCache() const { return m_cache; }
	int cacheId() const { return m_id; }
	void bindToCache(TextureCache &) const;
	void unbindFromCache() const;

	virtual void cacheUpload(Texture &) const = 0;
	virtual int2 textureSize() const = 0;

  private:
	void onCacheDestroy();
	friend class TextureCache;

	mutable TextureCache *m_cache; // TODO: remove, use TextureCache::main_cache?
	mutable int m_id;
};

// accesed textures are valid only until nextFrame, if the texture
// was in the atlas, it's data might change, so accessTexture should be called every frame
//
// TODO: texture packing in the atlas can be greatly improved:
// - instead of round robin, select next AtlasNode based on active texture count, free space, etc.
// - when packing textures, use some smarter algorithm to save space
// - use PBO to copy textures from res.device_texture, and not from system memory
class TextureCache {
  public:
	TextureCache(int max_bytes);
	TextureCache(const TextureCache &) = delete;
	void operator=(const TextureCache &) = delete;
	~TextureCache();

	bool isValidId(int res_id) const { return res_id >= 0 && res_id < size(); }
	int size() const { return (int)m_resources.size(); }

	void unload(int res_id);
	STexture access(int res_id, bool put_in_atlas, FRect &);
	STexture atlas() { return m_atlas; }

	void setMemoryLimit(int bytes) { m_memory_limit = bytes; }
	int memoryLimit() const { return m_memory_limit; }
	int memorySize() const { return m_memory_size; }
	void nextFrame();

	static TextureCache main_cache;

  protected:
	int add(CachedTexture *ptr, const int2 &size);
	void remove(int res_id);
	friend class CachedTexture;

  protected:
	struct Resource {
		CachedTexture *res_ptr;
		STexture device_texture;

		int2 size, atlas_pos;
		int atlas_node_id;
		int last_update;

		ListNode main_node;  // shared by two lists: m_main_list, m_free_list
		ListNode atlas_node; // shared by two lists: AtlasNode::list, m_atlas_queue
	};

	enum { node_size = 256 };

	struct AtlasNode {
		IRect rect;
		List list;
	};

	PodArray<AtlasNode> m_atlas_nodes;
	STexture m_atlas;
	int2 m_atlas_size;
	int m_atlas_counter;

	vector<Resource> m_resources;
	int m_memory_limit;
	int m_memory_size;
	int m_last_update;
	int m_last_node;

	List m_main_list;
	List m_free_list;
	List m_atlas_queue;
};

#endif
