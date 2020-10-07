//*********************************************************************************************************
//Collision Test Header
//Defines/declares CollisionTest functions
//Used to test collisions of sprite with Mappy blocks
//*********************************************************************************************************
#ifndef COLLISION_TEST_H
#define COLLISION_TEST_H
#include <string>
#include "sprite.h"

//Test right collision
bool rCol(SPRITE *spr);

//Test left collision
bool lCol(SPRITE *spr);

//Test top collision
bool tCol(SPRITE *spr);

//Test right collision
bool bCol(SPRITE *spr);

//Primary collision test function
bool col(SPRITE *spr, std::string s);

//Test right collision
bool rCol(SPRITE *spr) {
	BLKSTR *blockdata;
	blockdata = MapGetBlockInPixels(spr->x + spr->width*.90, spr->y + spr->height / 2);
	if (blockdata->tl || blockdata->user1) {
		return true;
	}
	else {
		return false;
	}
}

//Test left collision
bool lCol(SPRITE *spr) {
	BLKSTR *blockdata;
	blockdata = MapGetBlockInPixels(spr->x + spr->width*.10, spr->y + spr->height / 2);
	if (blockdata->tl || blockdata->user1) {
		return true;
	}
	else {
		return false;
	}
}

//Test top collision
bool tCol(SPRITE *spr) {
	BLKSTR *blockdata;
	blockdata = MapGetBlockInPixels(spr->x + spr->width*.5, spr->y);
	if (blockdata->tl) {
		return true;
	}
}

//Test right collision
bool bCol(SPRITE *spr) {
	BLKSTR *blockdata;
	blockdata = MapGetBlockInPixels(spr->x + spr->width*.5, spr->y + spr->height);
	if (blockdata->tl) {
		return true;
	}
}

//Primary collision test function
//col(SPRITE *spr, int i) - Accepts a sprite and and string "t" "b" "l" or "r" to test top bottom left or right block collision
bool col(SPRITE *spr, std::string s) {
	if (s == "t") {
		if (tCol(spr) == true) {
			return true;
		}
	}
	if (s == "b") {
		if (bCol(spr) == true) {
			return true;
		}
	}
	if (s == "l") {
		if (lCol(spr) == true) {
			return true;
		}
	}
	if (s == "r") {
		if (rCol(spr) == true) {
			return true;
		}
	}
	else {
		return false;
	}
}
#endif