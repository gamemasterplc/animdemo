#ifndef ANIMSPRITE_FORMAT_H
#define ANIMSPRITE_FORMAT_H

#include <stdint.h>

typedef struct aspr_frame_data {
	uint16_t time;
	uint16_t sprite_idx;
} ASPRFrameData;

typedef struct aspr_anim {
	char *name;
	uint16_t num_frames;
	uint16_t total_time;
	ASPRFrameData frames[];
} ASPRAnim;

typedef struct aspr_sprite_data {
	uint32_t spr_max_size;
	void *sprite[];
} ASPRSpriteData;

typedef struct aspr_data {
	uint32_t magic;
	uint32_t anim_count;
	uint32_t sprite_count;
	ASPRSpriteData *sprite_data;
	ASPRAnim *anims[];
} ASPRData;

#endif