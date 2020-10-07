//*********************************************************************************************************
//Grabframe Header
//Defines/declares Grabframe function
//Used to grab cropped frames from a sprite sheet and return them
//*********************************************************************************************************
#ifndef GRABFRAME_H
#define GRABFRAME_H
#include "allegro.h"
BITMAP *grabframe(BITMAP *source, int width, int height, int startx, int starty, int columns, int frame);

//compiled_grabframe(BITMAP *source, int width, int height, int startx, int starty, int colums, int frame) - Source bitmap, it's width, height, where it starts x, and y, how many colums, and current frame
//Accepts a bitmap and parses it to divide it into individual frames for animation
BITMAP *grabframe(BITMAP *source,
	int width, int height,
	int startx, int starty,
	int columns, int frame)
{
	BITMAP *sprite = create_bitmap(width, height); //Hold compiled sprite
	BITMAP *temp = create_bitmap(width, height); //Temporary Bitmap to copy to

	int x = startx + (frame % columns) * width; //End x
	int y = starty + (frame / columns) * height; //End y of where to cut to

	blit(source, temp, x, y, 0, 0, width, height); //Blit the cropped frame to temp bitmap

	//Set compiled sprite from temp to sprite
	blit(temp, sprite, 0, 0, 0, 0, width, height);

	//Destroy temp bitmap to be reused again
	destroy_bitmap(temp);

	//Return the cropped frame as sprite
	return sprite;
}
#endif