#include "libdragon.h"
#include "animsprite.h"
#include "asprformat.h"

#define PTR_DECODE(base, ptr) ((void*)(((uint8_t*)(base)) + (uint32_t)(ptr)))

#define MAX_STREAM_BUFS 2

typedef struct anim_sprite {
	ASPRData *data;
	int anim_idx;
	int frame_idx;
	float time;
	sprite_t *stream_buf[MAX_STREAM_BUFS];
	int buffer;
	uint32_t sprite_romofs;
	bool loop;
	bool pause;
	float speed;
} AnimSprite;

static ASPRData *LoadASPR(const char *path)
{
	int sz;
    ASPRData *data = asset_load(path, &sz);
	
	for(uint32_t i=0; i<data->anim_count; i++) {
		data->anims[i] = PTR_DECODE(data, data->anims[i]);
	}
	for(uint32_t i=0; i<data->anim_count; i++) {
		data->anims[i]->name = PTR_DECODE(data, data->anims[i]->name);
	}
	if(data->sprite_data) {
		data->sprite_data = PTR_DECODE(data, data->sprite_data);
		for(uint32_t i=0; i<data->sprite_count+1; i++) {
			data->sprite_data->sprite[i] = PTR_DECODE(data, data->sprite_data->sprite[i]);
		}
	}
	data_cache_hit_writeback_invalidate(data, sz);
	return data;
}

static uint32_t GetImageIdx(AnimSprite *sprite)
{
	ASPRAnim *anim = sprite->data->anims[sprite->anim_idx];
	return anim->frames[sprite->frame_idx].sprite_idx;
}

static void UpdateSpriteFrame(AnimSprite *sprite)
{
	if(sprite->data->sprite_data) {
		return;
	}
	uint32_t base_ofs = sprite->sprite_romofs;
	uint32_t ofs = base_ofs+offsetof(ASPRSpriteData, sprite);
	uint32_t image_ofs[2];
	uint32_t image = GetImageIdx(sprite);
	ofs += sizeof(uint32_t)*image;
	data_cache_hit_writeback_invalidate(&image_ofs, sizeof(image_ofs));
    dma_read(&image_ofs, ofs, sizeof(image_ofs));
	dma_read(sprite->stream_buf[sprite->buffer], base_ofs+image_ofs[0], image_ofs[1]-image_ofs[0]);
	sprite->buffer++;
	if(sprite->buffer >= MAX_STREAM_BUFS) {
		sprite->buffer = 0;
	}
}

AnimSprite *AnimSpriteLoad(const char *path)
{
	assertf(strncmp(path, "rom:/", 5) == 0, "Cannot open %s: File must be in ROM (rom:/)", path);
	AnimSprite *sprite = malloc(sizeof(AnimSprite));
	
	sprite->data = LoadASPR(path);
	sprite->anim_idx = 0;
	sprite->frame_idx = 0;
	sprite->time = 0;
	sprite->sprite_romofs = 0;
	sprite->loop = false;
	sprite->pause = false;
	sprite->speed = 1.0f;
	
	
	if(sprite->data->sprite_data == NULL) {
		char path_buf[strlen(path)+5];
		strcpy(path_buf, path);
		strcat(path_buf, ".dat");
		sprite->sprite_romofs = dfs_rom_addr(path_buf+5);
		assertf(sprite->sprite_romofs != 0, "File %s missing", path_buf);
		sprite->buffer = 0;
		uint32_t data_size;
		data_cache_hit_writeback_invalidate(&data_size, sizeof(data_size));
		dma_read(&data_size, sprite->sprite_romofs+offsetof(ASPRSpriteData, spr_max_size), sizeof(data_size));
		for(size_t i=0; i<MAX_STREAM_BUFS; i++) {
			sprite->stream_buf[i] = malloc_uncached(data_size);
		}
	}
	UpdateSpriteFrame(sprite);
	return sprite;
}

void AnimSpriteDelete(AnimSprite *sprite)
{
	if(sprite->data->sprite_data) {
		for(size_t i=0; i<MAX_STREAM_BUFS; i++) {
			free_uncached(sprite->stream_buf[i]);
		}
	}
	
	free(sprite->data);
	free(sprite);
}

static int32_t SearchAnim(ASPRData *data, const char *name)
{
	for(uint32_t i=0; i<data->anim_count; i++) {
		if(!strcmp(data->anims[i]->name, name)) {
			return i;
		}
	}
	return -1;
}

void AnimSpriteSetAnim(AnimSprite *sprite, const char *name)
{
	int32_t anim_idx = SearchAnim(sprite->data, name);
	assertf(anim_idx != -1, "No animation named %s exists.", name);
	if(sprite->anim_idx != anim_idx) {
		sprite->anim_idx = anim_idx;
		sprite->time = 0;
		sprite->frame_idx = 0;
		UpdateSpriteFrame(sprite);
	}
}

void AnimSpriteSetLoop(AnimSprite *sprite, bool loop)
{
	sprite->loop = loop;
}

void AnimSpriteSetTime(AnimSprite *sprite, float time)
{
	sprite->time = time;
	sprite->frame_idx = 0;
}

void AnimSpriteSetSpeed(AnimSprite *sprite, float speed)
{
	assertf(speed >= 0, "Speed must be non-negative");
	sprite->speed = speed;
}

float AnimSpriteGetTime(AnimSprite *sprite)
{
	return sprite->time;
}

void AnimSpriteUpdate(AnimSprite *sprite, float dt)
{
	ASPRAnim *anim = sprite->data->anims[sprite->anim_idx];
	float speed = sprite->speed;
	if(speed == 0.0f || sprite->pause) {
		return;
	}
	sprite->time += speed*dt;
	while(sprite->loop && sprite->time > anim->total_time) {
		sprite->time -= anim->total_time;
		sprite->frame_idx = 0;
	}
	if(sprite->frame_idx < anim->num_frames+1) {
		int old_frame = sprite->frame_idx;
		while(sprite->time > anim->frames[sprite->frame_idx+1].time) {
			sprite->frame_idx++;
			if(sprite->frame_idx >= anim->num_frames+1) {
				break;
			}
		}
		if(sprite->frame_idx != old_frame) {
			UpdateSpriteFrame(sprite);
		}
		
	}		
}

sprite_t *AnimSpriteGetSpriteCurr(AnimSprite *sprite)
{
	int buffer;
	if(sprite->data->sprite_data) {
		return sprite->data->sprite_data->sprite[GetImageIdx(sprite)];
	}
	buffer = sprite->buffer;
	if(buffer == 0) {
		buffer = MAX_STREAM_BUFS-1;
	} else {
		buffer--;
	}
	return sprite->stream_buf[buffer];
}
