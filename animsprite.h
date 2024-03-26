#ifndef ANIMSPRITE_H
#define ANIMSPRITE_H

#include "libdragon.h"

typedef struct anim_sprite AnimSprite;

AnimSprite *AnimSpriteLoad(const char *path);
void AnimSpriteDelete(AnimSprite *sprite);
void AnimSpriteSetAnim(AnimSprite *sprite, const char *name);
void AnimSpriteSetLoop(AnimSprite *sprite, bool loop);
void AnimSpriteSetPause(AnimSprite *sprite, bool pause);

void AnimSpriteSetTime(AnimSprite *sprite, float time);
void AnimSpriteSetSpeed(AnimSprite *sprite, float time);
float AnimSpriteGetTime(AnimSprite *sprite);

void AnimSpriteUpdate(AnimSprite *sprite, float dt);
sprite_t *AnimSpriteGetSprite(AnimSprite *sprite);

#endif