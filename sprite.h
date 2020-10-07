//*********************************************************************************************************
//Sprite Header
//Defines SPRITE class
//*********************************************************************************************************


#ifndef SPRITE_H
#define SPRITE_H
#include <string>
//Sprite structure
typedef struct SPRITE
{
	double minX, minY, maxX, maxY, x, y, xVelocity, yVelocity; //Hold xy position and velocities
	int width, height; //Hold width and height sprite
	int xdelay, ydelay; //Hold x and y delays, slows down movement in these directions if needed
	int xcount, ycount; //Count the current x and y, used to compare against delay, if delay is reached, then movement will take place
	int curframe, maxframe, animdir; //Holds current frame, max frame of animation, and the direction of animation
	int framecount, framedelay; //Holds which frame to display, delay is used like x and y delay, slows animation frames down if desired
	int state; //Determine what state the sprite is in
	int facing; //Determine direction sprite is facing 0 = front, 1 = left, 2 = right
	int dir; //Determine directoin sprite is going 0 = not moving, 1 = left, 2 = right
	int jumps; //Determines how many jumps sprite has made
	int hp; //Health
	std::string type; //Holds the type of sprite, heart, potion, etc.
	bool alive;
	int knockback;

}SPRITE;
#endif