/********************************************************************************************************************
Author: Chad Cromwell
Date: February 27th, 2017
Assignment: 4
Program: Glorp.cpp
Description: An Allegro based platformer made with Allegro, Mappy, and C++ with added AI, threading, and using .dat files.
Copyright info:
Graphics: Chad Cromwell
Sounds: bg.wav is "Searching" by Eric Skiff retrieved from http://ericskiff.com/music/ Creative Commons Attribution License: http://ericskiff.com/music/
Fonts: titleFont is Tralfamadore by Apostrophic Labs retrieved from http://www.1001fonts.com/tralfamadore-font.html under Donationware(Freeware) license: http://www.1001fonts.com/tralfamadore-font.html#license
bodyFont is Arial font under Windows EULA
No TTfonts are distributed with this game, only .pcx glyphs
********************************************************************************************************************/

#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <string>
#include "allegro.h"
#include "mappyal.h"
#include "collisiontest.h"
#include "sprite.h"
#include "grabframe.h"

#define BLACK makecol(0,0,0)
#define WHITE makecol(255,255,255)
#define WIDTH 1024
#define HEIGHT 768
#define GRAVITY 1;
#define TERMINAL_VELOCITY 15

DATAFILE *datFile = NULL;

//**Timing***
volatile long counter = 0;
void Increment() {
	counter++;
}
END_OF_FUNCTION(Increment)

//Timing variables
int ticks; //Keep track of frames
int seconds; //Keep track of seconds
int mins; //Keep track of minutes
		  //************

		  //***Variables***
		  //Booleans
bool canJump = true; //Determine if Sprite Guy is allowed to jump
bool jumping = false; //Determine if Sprite Guy is jumping
bool rightRelease = false; //Determine if right key has been released
bool leftRelease = false; //Determine if left key has been released
bool spaceRelease = false; //Determine if space bar has been released
bool knockback = false; //Determine if knockback needs to be applied
bool loseHP = false; //Determine if HP is lost
bool dead = false;  //Determine if player dies
bool menu = false;  //Determine if in menu
bool gameOver = false; //Determine if game loop should exit
bool mute = false;  //Determine if music is muted or not
bool help = false; //Determine if help is toggled or not
bool scoreScreen = false; //Determine if need to go to score screen
bool stop = false; //Determine if player needs to be stopped

				   //Ints/Doubles
int mapXOff, mapYOff; //Map offsets
int hp = 3; //Initialize hit points
int lastHP = 0; //Initialize lastHP to keep track of hp will invincible
int timer = 0; //Initialize timer to keep track of time passed
int volume; //Volume
int shipParts = 0; //Keep track of how many ship parts collected
int xContact, yContact; //Contact position, used for knockback calculation
int knockVelocity = 15; //Velocity of knockback
double rotate = 0; //Used for rotation angle of Glorp's rolling animation

				   //BITMAPS
BITMAP *buffer; //For screen buffer
BITMAP *hpHeart; //For HP heart
BITMAP *tempBuf;

//Physics variables
double gravity = 1; //Amount of gravity, good setting is 1
double jumpVelocity = -15; //Initial velocity of jump, good setting is -15
double glorpAcceleration = .7; //Acceleration of Sprite Guy while running, increase this and he accelerates faster. If set below 0 he will move backwards. Keep above 0.
double glorpDecceleration = .5; //Decceleration of Sprite Guy when stopping running, increase this and he slows down faster. Keep above 0.
double maxHVelocity = 18; //Maximum velocity that Sprite Guy can run, the higher the faster, the lower the slower. Keep above 0.
double drag = .5; //Air resistance, how fast Sprite Guy slows down when running, best set to 50% of glorpAcceleration
double maxDropVelocity = 3; //Egg terminal velocity
double terminalVelocity = TERMINAL_VELOCITY; //Set terminalVelocity to constant declared above

											 //SAMPLES/Sound variables
SAMPLE *bg;
SAMPLE *hurt;
SAMPLE *healed;
SAMPLE *collectable;

//FONTS
FONT *bodyFont;
FONT *titleFont;

//******************************************************************************************************************************

//***Sprites***
//Glorp
BITMAP *glorpImg[20];
SPRITE glorp1; //Sprite
SPRITE *glorp = &glorp1; //Pointer

						 //Part 1
BITMAP *partImg[2];
SPRITE part1;
SPRITE *part = &part1;

//Part 2
SPRITE part21;
SPRITE *part2 = &part21;

//Part 3
SPRITE part31;
SPRITE *part3 = &part31;

//Evil Cheese
BITMAP *cheeseImg[2];
SPRITE cheese1;
SPRITE *cheese = &cheese1;

SPRITE cheese21;
SPRITE *cheese2 = &cheese21;

SPRITE cheese31;
SPRITE *cheese3 = &cheese31;

SPRITE cheese41;
SPRITE *cheese4 = &cheese41;

SPRITE cheese51;
SPRITE *cheese5 = &cheese51;

SPRITE cheese61;
SPRITE *cheese6 = &cheese61;

SPRITE cheese71;
SPRITE *cheese7 = &cheese71;

SPRITE cheese81;
SPRITE *cheese8 = &cheese81;

SPRITE cheese91;
SPRITE *cheese9 = &cheese91;

SPRITE cheese101;
SPRITE *cheese10 = &cheese101;

SPRITE cheese111;
SPRITE *cheese11 = &cheese111;

//Hearts
BITMAP *heartImg[1];
SPRITE heart1;
SPRITE *heart = &heart1;

SPRITE heart21;
SPRITE *heart2 = &heart21;

//Ship
BITMAP *shipImg[2];
SPRITE ship1;
SPRITE *ship = &ship1;
//*************

//Thread MUTEX to protect variables in use
pthread_mutex_t threadsafe = PTHREAD_MUTEX_INITIALIZER;

//maskSGCollisionTest(SPRITE *spr1) - Test to see if objects collide with Sprite Guy with mask, more closely bounds to his body instead of BITMAP box
bool maskedCollisionTest(SPRITE *spr1) {
	if (spr1->x + spr1->width < glorp->x || spr1->x > glorp->x + glorp->width || spr1->y > glorp->y + glorp->height || spr1->y + spr1->height < glorp->y) {

		return false;
	}
	else {
		return true;
	}
}

//drawLevel(BITMAP *dest) - *dest = destination Bitmap. Draws background and ground
void drawLevel(BITMAP *dest) {
	MapDrawBG(buffer, mapXOff, mapYOff, 0, 0, WIDTH - 1, HEIGHT - 1);
	MapDrawFG(buffer, mapXOff, mapYOff, 0, 0, WIDTH - 1, HEIGHT - 1, 0);
}


//physics(SPRITE *spr) - *spr = sprite. Accepts SPRITE and applies physics to it
void physics(SPRITE *spr) {
	//Update his size
	glorp->height = 32;
	glorp->width = 32;

	//Update ycount
	if (++spr->ycount > spr->ydelay) {
		spr->ycount = 0;

		//If player is not on the ground, apply gravity
		if (col(spr, "b") == false) {
			gravity = GRAVITY; //Set gravity
			if (spr->yVelocity > terminalVelocity) { //If player is falling faster than terminal velocity
				gravity = 0; //Set no gravity
				spr->yVelocity = terminalVelocity; //Set velocity to max terminal velocity
			}
			stop = true;
		}

		//If player is on the ground
		if (col(spr, "b") == true) {
			gravity = 0;
			if (stop) {
				spr->yVelocity = 0;
				stop = false;
			}
		}

		if (col(spr, "t") == true) {
			spr->yVelocity = 0;
		}

		spr->yVelocity = spr->yVelocity + gravity;
		spr->y += spr->yVelocity;
	}


	//Update x position
	if (++spr->xcount > spr->xdelay) { //Increment the sprite's xcount and if it is bigger than xdelay

		spr->xcount = 0; //Set xcount to 0

						 //If xVelocity is at maxVelocity
		if (spr->xVelocity > maxHVelocity) {
			spr->xVelocity = maxHVelocity; //Set velocity as max, stops him from going faster
		}

		//Same as above but for moving left
		if (spr->xVelocity < -maxHVelocity) {
			spr->xVelocity = -maxHVelocity;
		}

		//If he's moving right
		if (spr->xVelocity > 0) {
			spr->xVelocity = spr->xVelocity - drag; //Apply drag
			spr->x += spr->xVelocity; //Update velocity
			if (spr->xVelocity <= 0) { //When he stops moving due to drag
				spr->xVelocity = 0; //Stop his velocity
			}
		}

		//If he's moving left, same logic as above
		if (spr->xVelocity < 0) {
			spr->xVelocity = spr->xVelocity + drag;
			spr->x += spr->xVelocity;
			if (spr->xVelocity >= 0) {
				spr->xVelocity = 0;
			}
		}

		//If player gets hit from the RIGHT, knock player to the left with knock velocity
		if (xContact > spr->x + spr->width / 2) {
			if (knockback == true) {
				if (col(spr, "l") == true) { //If there is a block to the left of the player, only send them upwards
					spr->yVelocity = -knockVelocity / 2;
					if (spr->y < yContact - spr->height * 2) {
						knockback = false;
						spr->hp--;
					}
				}
				else { //Otherwise, knowck them back as normal
					int knock = -knockVelocity;
					spr->xVelocity = knock;
					spr->yVelocity = knock / 2;
					if (spr->x < xContact - spr->width*1.5) { //Knock player back until they are two sprite widths away from the contact point
						knockback = false;
						spr->hp--;
					}
				}
			}
		}

		//If player gets hit from the LEFT, knock player to the right with knock velocity
		if (xContact < spr->x + spr->width / 2) {
			if (knockback == true) {
				if (col(spr, "r") == true) { //If there is a block to the right of the player, only send them upwards
					spr->yVelocity = -knockVelocity / 2;
					if (spr->y < yContact - spr->height * 2) {
						knockback = false;
						spr->hp--;
					}
				}
				else { //Otherwise, knock them back as normal
					int knock = +knockVelocity;
					spr->xVelocity = knock;
					spr->yVelocity = -knock / 2;
					if (spr->x > xContact + spr->width*1.5) { //Knock player back until they are two sprite widths away from the contact point
						knockback = false;
						spr->hp--;
					}
				}
			}
		}
	}

}

//draw(BITMAP *s, COMPILED_SPRITE *cs[], SPRITE *c) - *s destination bitmap, source compiled sprite frame, sprite for x and y location - Accepts these parameters and draws it to the screen
void draw(BITMAP *s, COMPILED_SPRITE *cs[], SPRITE *c) {
	draw_compiled_sprite(s, cs[c->curframe], c->x, c->y);
}

//animate(BITMAP *dest, SPRITE *spr) - Animates sprite to destination bitmap
void animate(BITMAP *dest, SPRITE *spr) {

	//If in NORMAL or INVINCIBLE form

	//If going right
	if (spr->dir == 1) {
		//If the frame is outside of right walking animation, go back to 0 animation frame (0-10 is right walking animation)
		if (spr->curframe > 10) {
			spr->curframe = 0;
		}
		if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
			spr->framecount = 0; //Set framecount to 0
			if (++spr->curframe > 9) { //Increment the sprite's curframe, if it is greater than 19
				spr->curframe = 9; //Set sprite's curframe to 10, to keep animating through his running right animation
			}
		}
		if (spr->curframe < 9) {
			//draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
			draw_sprite(buffer, glorpImg[spr->curframe], spr->x - mapXOff, spr->y);
		}
		if (spr->curframe == 9) {
			rotate += 10;
			rotate_sprite(buffer, glorpImg[spr->curframe], spr->x - mapXOff, spr->y, itofix(rotate));
			if (rotate > 256) {
				rotate = 0;
			}
		}

	}

	//If going left
	if (spr->dir == 2) {

		//If the frame is outside of the left walking animation, set to 20 (10-20 is left walking animation)
		if (spr->curframe < 10) {
			spr->curframe = 10;
		}
		if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
			spr->framecount = 0; //Set framecount to 0
			if (++spr->curframe > 19) { //Increment the sprite's curframe, if it is greater than 39
				spr->curframe = 19; //Set sprite's curframe to 30, to keep animating him through his running left animation
			}
		}

		if (spr->curframe < 19) {
			//draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
			draw_sprite(buffer, glorpImg[spr->curframe], spr->x - mapXOff, spr->y);
		}

		if (spr->curframe == 19) {
			rotate -= 10;
			rotate_sprite(buffer, glorpImg[spr->curframe], spr->x - mapXOff, spr->y, itofix(rotate));
			if (rotate < -256) {
				rotate = 0;
			}
		}
	}

	//If stopping going right, animate him back to facing the screen
	if (spr->dir == 0 && spr->facing == 1) { //If not moving and facing right
		if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
			spr->framecount = 0; //Set framecount to 0
			if (--spr->curframe < 0) { //Decrement curframe until it reaches 0, which is him facing the screen
				spr->curframe = 0; //Set sprite's curframe to 0
				spr->facing = 0;
			}
		}
		//draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
		draw_sprite(buffer, glorpImg[spr->curframe], spr->x - mapXOff, spr->y);
	}

	//If stopping going left, animate him back to facing the screen
	if (spr->dir == 0 && spr->facing == 2) { //If not moving and facing left
		if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
			spr->framecount = 0; //Set framecount to 0
			if (--spr->curframe < 10) { //Decrement curframe until frame reaches 20, which is him facing the screen
				spr->curframe = 10; //Set sprite's curframe to 20
				spr->facing = 0;
			}
		}
		//draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
		draw_sprite(buffer, glorpImg[spr->curframe], spr->x - mapXOff, spr->y);
	}

	//If facing the front of the screen
	if (spr->dir == 0 && spr->facing == 0) { //If not moving and facing front
		if (spr->curframe == 9) { //If the curframe is 19
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 0) { //Decrement curframe until he's facing the screen
					spr->curframe = 0; //Set sprite's curframe to 0
				}
			}
		}
		if (spr->curframe == 19) { //If the curframe is 39
			if (++spr->framecount > spr->framedelay) { //Increment the sprite's framecount, if it is bigger than the frame delay
				spr->framecount = 0; //Set framecount to 0
				if (--spr->curframe < 10) { //Decrement curframe until he's facing the screen
					spr->curframe = 10; //Set sprite's curframe to 0
				}
			}
		}
		//draw(buffer, spriteGuyImg, spr); //Draw him to the buffer
		draw_sprite(buffer, glorpImg[spr->curframe], spr->x - mapXOff, spr->y);
	}
}

//Spawns a sprite and it paces LR between minX and maxX, or in place if 0 acceleration or terminal velocity is given
void spawnPacer(BITMAP *dest, BITMAP *b[], SPRITE *spr, double a, double tv) {

	//If the sprite is alive
	if (spr->alive == true) {
		//Increment framecount, if it gets above framedelay, set back to 0
		if (++spr->framecount > spr->framedelay) {
			spr->framecount = 0;

			//Animate
			if (++spr->curframe > spr->maxframe) {
				spr->curframe = 0;
			}

		}

		//Going right
		if (spr->dir == 1) {
			spr->xVelocity += a; //Accelerate the sprite
			if (spr->xVelocity > tv) { //If terminal velocity is reached
				spr->xVelocity = tv; //Limit speed to terminal velocity
			}
			spr->x = spr->x + spr->xVelocity; //Change x position

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

																			 //If the sprite reaches the max X position, stop it and send it the other way
			if (spr->x > spr->maxX) {
				spr->xVelocity = 0;
				spr->dir = 2;
			}
		}

		//Going left
		if (spr->dir == 2) {
			spr->xVelocity += -a; //Accelerate the sprite
			if (spr->xVelocity < -tv) { //If terminal velocity is reached
				spr->xVelocity = -tv; //Limit speed to terminal velocity
			}
			spr->x = spr->x + spr->xVelocity; //Change x position

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

																			 //If the sprite reaches min X position, stop it and send it the other way
			if (spr->x < spr->minX) {
				spr->xVelocity = 0;
				spr->dir = 1;
			}
		}

		//Going down - Same logic as above
		if (spr->dir == 3) {
			spr->yVelocity += a;
			if (spr->yVelocity > tv) {
				spr->yVelocity = tv;
			}
			spr->y = spr->y + spr->yVelocity;

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

			if (spr->y > spr->maxY) {
				spr->yVelocity = 0;
				spr->dir = 4;
			}
		}

		//Going up - Same logic as above
		if (spr->dir == 4) {
			spr->yVelocity += -a; //Accelerate the sprite
			if (spr->yVelocity < -tv) { //If terminal velocity is reached
				spr->yVelocity = -tv; //Limit speed to terminal velocity
			}
			spr->y = spr->y + spr->yVelocity; //Change x position

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

			if (spr->y < spr->minY) {
				spr->yVelocity = 0;
				spr->dir = 3;
			}
		}

		if (maskedCollisionTest(spr) == true) { //If collided with player

												//If sprite is of type heart
			if (spr->type == "heart") {
				play_sample(healed, 255, 127, 1000, FALSE); //Play healed sound effect

				if (hp < 3) { //If less than HP
					hp++; //Increase HP
				}
				spr->alive = false; //No longer alive, so it will no longer be drawn
			}

			//If sprite is of type hurt and knockback is false
			if (spr->type == "hurt" && knockback == false) {
				play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
				xContact = spr->x + spr->width / 2; //Set xContact position
				yContact = spr->y + spr->height / 2; //Set yContact position
				knockback = true; //Set knockback to true so player gets knocked back
			}

			//If sprite is of type collectable
			if (spr->type == "collectable") {
				play_sample(collectable, 255, 127, 1000, FALSE); //Play collectable sound effect
				spr->alive = false; //No longer alive, so it will no longer be drawn
				shipParts++; //Increment shipParts count
			}

			//If sprite is of type end
			if (spr->type == "end") {
				scoreScreen = true; //Go to score screen becase they have reached the end
			}

		}
	}

}
//AI Implementation
//Spawns a sprite that paces but will move faster if the player comes closer to it
void spawnHomingPacer(BITMAP *dest, BITMAP *b[], SPRITE *spr, double a, double tv) {

	//If player comes within + or - 150 of the NPC, it will move twice as fast.
	if (glorp->x > spr->x - 150 && glorp->x < spr->x + 150) {
		a = a * 2;
		tv = tv * 2;
	}
	//If the sprite is alive
	if (spr->alive == true) {
		//Increment framecount, if it gets above framedelay, set back to 0
		if (++spr->framecount > spr->framedelay) {
			spr->framecount = 0;

			//Animate
			if (++spr->curframe > spr->maxframe) {
				spr->curframe = 0;
			}

		}

		//Going right
		if (spr->dir == 1) {
			spr->xVelocity += a; //Accelerate the sprite
			if (spr->xVelocity > tv) { //If terminal velocity is reached
				spr->xVelocity = tv; //Limit speed to terminal velocity
			}
			spr->x = spr->x + spr->xVelocity; //Change x position

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

																			 //If the sprite reaches the max X position, stop it and send it the other way
			if (spr->x > spr->maxX) {
				spr->xVelocity = 0;
				spr->dir = 2;
			}
		}

		//Going left
		if (spr->dir == 2) {
			spr->xVelocity += -a; //Accelerate the sprite
			if (spr->xVelocity < -tv) { //If terminal velocity is reached
				spr->xVelocity = -tv; //Limit speed to terminal velocity
			}
			spr->x = spr->x + spr->xVelocity; //Change x position

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

																			 //If the sprite reaches min X position, stop it and send it the other way
			if (spr->x < spr->minX) {
				spr->xVelocity = 0;
				spr->dir = 1;
			}
		}

		//Going down - Same logic as above
		if (spr->dir == 3) {
			spr->yVelocity += a;
			if (spr->yVelocity > tv) {
				spr->yVelocity = tv;
			}
			spr->y = spr->y + spr->yVelocity;

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

			if (spr->y > spr->maxY) {
				spr->yVelocity = 0;
				spr->dir = 4;
			}
		}

		//Going up - Same logic as above
		if (spr->dir == 4) {
			spr->yVelocity += -a; //Accelerate the sprite
			if (spr->yVelocity < -tv) { //If terminal velocity is reached
				spr->yVelocity = -tv; //Limit speed to terminal velocity
			}
			spr->y = spr->y + spr->yVelocity; //Change x position

			draw_sprite(buffer, b[spr->curframe], spr->x - mapXOff, spr->y); //Draw the sprite

			if (spr->y < spr->minY) {
				spr->yVelocity = 0;
				spr->dir = 3;
			}
		}

		if (maskedCollisionTest(spr) == true) { //If collided with player

												//If sprite is of type heart
			if (spr->type == "heart") {
				play_sample(healed, 255, 127, 1000, FALSE); //Play healed sound effect

				if (hp < 3) { //If less than HP
					hp++; //Increase HP
				}
				spr->alive = false; //No longer alive, so it will no longer be drawn
			}

			//If sprite is of type hurt and knockback is false
			if (spr->type == "hurt" && knockback == false) {
				play_sample(hurt, 255, 127, 1000, FALSE); //Play hurt sound effect
				xContact = spr->x + spr->width / 2; //Set xContact position
				yContact = spr->y + spr->height / 2; //Set yContact position
				knockback = true; //Set knockback to true so player gets knocked back
			}

			//If sprite is of type collectable
			if (spr->type == "collectable") {
				play_sample(collectable, 255, 127, 1000, FALSE); //Play collectable sound effect
				spr->alive = false; //No longer alive, so it will no longer be drawn
				shipParts++; //Increment shipParts count
			}

			//If sprite is of type end
			if (spr->type == "end") {
				scoreScreen = true; //Go to score screen becase they have reached the end
			}

		}
	}

}


//controller(SPRITE *spr) - *spr = SPRITE. A controller that moves Sprite Guy, if left or right is pressed it moves accordingly, if space is pressed Sprite Guy jumps
void controller(SPRITE *spr) {

	//Keep player within the map
	if (spr->y < spr->height) { //If player hits top of map
		spr->yVelocity = 0;
	}
	if (spr->y > HEIGHT - spr->height) { //If player hits bottom of map (falls through hole)
		spr->yVelocity = 0; //Set yVelocity to 0
		gravity = 0; //Turn off gravity to stop falling
		spr->y = HEIGHT - 64; // Keep player within screen for death screen
		dead = true; //Set to dead, go to death screen
	}
	if (spr->x < 0) { //If player runs into the edge of the map
		spr->xVelocity = 0; //Stop velocity
		spr->x = 0; //Prevent from going further than 0x
	}
	if (spr->x > mapwidth*mapblockwidth - spr->width) { //If player runs into the edge of the map
		spr->xVelocity = 0; //Stop velocity
		spr->x = mapwidth*mapblockwidth - spr->width; //Prevent from going further than map width x;
	}

	//When a key is pressed
	if (keypressed()) {

		//When left is pressed
		if (key[KEY_LEFT] && !key[KEY_RIGHT]) { //Only if right key isn't pressed
			spr->facing = 2;
			spr->dir = 2;

			spr->xVelocity = spr->xVelocity - glorpAcceleration; //Apply acceleration
			leftRelease = true; //Set leftRelease to true so that we can apply decceleration when left is released

			if (col(spr, "l") == true) {
				spr->xVelocity = 0;
			}
		}

		//When right is pressed
		if (key[KEY_RIGHT] && !key[KEY_LEFT]) { //Only if left key isn't pressed
			spr->facing = 1;
			spr->dir = 1;

			spr->xVelocity = spr->xVelocity + glorpAcceleration; //Apply acceleration
			rightRelease = true; //Set rightRelease to true so that we can apply decceleration when right is released

			if (col(spr, "r") == true) { //If player runs into collidable block going right -10 so it doesn't check the bottom block
				spr->xVelocity = 0; //Stop velocity
			}
		}

		if (!key[KEY_LEFT] && !key[KEY_RIGHT] || key[KEY_LEFT] && key[KEY_RIGHT]) { //If no key is being pressed, or both keys are being pressed, we want to stop Sprite Guy
			spr->dir = 0;

			//If right key has been released
			if (rightRelease == true) {
				spr->xVelocity = spr->xVelocity - glorpDecceleration; //Apply decceleration
				if (spr->xVelocity <= 0) { //When velocity is 0
					rightRelease = false; //No longer apply decceleration
				}
			}
			//If left key has been released
			if (leftRelease == true) {
				spr->xVelocity = spr->xVelocity + glorpDecceleration; //Apply decceleration
				if (spr->xVelocity >= 0) { //When velocity is 0
					leftRelease = false; //No longer apply decceleration
				}
			}

			//Stop user from sliding off map sides
			if (col(spr, "l") == true) {
				spr->xVelocity = 0;
			}

			if (col(spr, "r") == true) { //If player runs into collidable block going right -10 so it doesn't check the bottom block
				spr->xVelocity = 0; //Stop velocity
			}

		}

		//When space bar is pressed, only allow one process of the space bar to happen
		if (key[KEY_SPACE] && spaceRelease == false) { //If the spaceRelease is false
			if (canJump == true) { //If player allowed to jump
				spr->yVelocity = 0; //Reset velocity to 0 before applying jump velocity - Must be done to negate phyysics/gravity
				spr->yVelocity = jumpVelocity; //Apply vertical velocity
				jumping = true; //Player is now jumping
				spaceRelease = true; //Set spaceRelease to true so this does not keep iterating through (if not done player will fly)
			}
		}

		//If space bar is not being pressed
		if (!key[KEY_SPACE]) {
			if (spaceRelease == true) { //If the space bar was just released
				spr->jumps++; //Increase the number of sprite jumps done
				spaceRelease = false; //Set spaceRelease to false so spacebar can now be pressed again
			}
		}

		//When ESC is pressed
		if (key[KEY_ESC]) {
			release_bitmap(buffer);
			gameOver = true;
		}

		//When CTRL+M is pressed - Unmute music
		if (mute == false && volume < 155 && key[KEY_LCONTROL] && key[KEY_M] || mute == false && volume < 155 && key[KEY_RCONTROL] && key[KEY_M]) {
			volume = 155;
			adjust_sample(bg, volume, 128, 1000, TRUE); //Unmute
			mute = true; //For key unpress check to allow it to be pressed again
		}

		//If unpress CTRL+M, allow it to be pressed again
		if (mute == true && !key[KEY_LCONTROL] && !key[KEY_M] || mute == true && !key[KEY_RCONTROL] && !key[KEY_M]) {
			mute = false;
		}

		//When CTRL+M is pressed - Mute music
		if (mute == false && volume > 0 && key[KEY_LCONTROL] && key[KEY_M] || mute == false && volume > 0 && key[KEY_RCONTROL] && key[KEY_M]) {
			volume = 0;
			adjust_sample(bg, volume, 128, 1000, TRUE); //Mute
			mute = true;
		}

		//update the map scroll position
		mapXOff = spr->x + spr->width / 2 - WIDTH / 2 + 10;
		mapYOff = spr->y + spr->height / 2 - HEIGHT / 2 + 10;

		//avoid moving beyond the map edge
		if (mapXOff < 0) mapXOff = 0;
		if (mapXOff >(mapwidth * mapblockwidth - WIDTH))
			mapXOff = mapwidth * mapblockwidth - WIDTH;
		if (mapYOff < 0)
			mapYOff = 0;
		if (mapYOff >(mapheight * mapblockheight - HEIGHT))
			mapYOff = mapheight * mapblockheight - HEIGHT;

	}

	//If player is not on the ground due to jumping
	if (!col(spr, "b") && jumping == true) {
		if (spr->jumps == 2) { //If player has completed 2 jumps
			canJump = false; //Player can no longer jump
		}
	}

	//When player reaches the ground
	if (col(spr, "b") && jumping == true && canJump == false) {
		jumping = false; //Player is no longer jumping
		canJump = true; //Player is allowed to jump
		spr->jumps = 0; //Reset player jumps to 0 to allow for double jump
	}

}

//score(int s) - Accepts int (counter) to determine how much time has passed
void score(BITMAP *dest) {
	textprintf_ex(dest, bodyFont, 0, 0, WHITE, -1, "Total parts: %d/3", shipParts); //Print to screen how much time has passed
}

void health(BITMAP *dest, SPRITE *spr) {
	if (spr->hp == 3) { //If player has 3 HP show 3 hearts
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 128, 5, 32, 32);
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 86, 5, 32, 32);
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 44, 5, 32, 32);
	}
	if (spr->hp == 2) { //If player has 2 HP show 2 hearts
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 86, 5, 32, 32);
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 44, 5, 32, 32);
	}
	if (spr->hp == 1) { //If player has 1 HP show 1 heart
		masked_blit(hpHeart, dest, 0, 0, SCREEN_W - 44, 5, 32, 32);

	}
	if (spr->hp == 0) { //If player has 0 HP, go to death screen
		dead = true;
		textprintf_ex(dest, bodyFont, SCREEN_W - text_length(bodyFont, "DEAD"), 0, WHITE, -1, "DEAD");
	}
}

//Thread0 - Threads the spawnPacer functions.
void* thread0(void* data) {
	int thread_id = *((int*)data);

	while (gameOver == false && dead == false) {

		if (pthread_mutex_lock(&threadsafe)) {
			allegro_message("Error: thread mutex was locked");
		}

		acquire_bitmap(tempBuf);
		//Draw the player's health
		health(tempBuf, glorp);

		//Display score
		score(tempBuf);
		release_bitmap(tempBuf);
		if (pthread_mutex_unlock(&threadsafe)) {
			allegro_message("ERROR: thread mutex unlock error");
		}

	}
	pthread_exit(NULL);
	return NULL;
}

//***Main***
int main(void)
{
	//initialize
	allegro_init();
	set_color_depth(24);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 1024, 768, 0, 0);
	install_timer();
	install_keyboard();
	srand(time(NULL));

	//Load dat file
	datFile = load_datafile("Glorp.dat");
	if (!datFile) {
		allegro_message("Could not load the datafile");
		allegro_exit();
	}

	int id;
	pthread_t pthread0;
	pthread_t pthread1;
	int threadid0 = 0;
	int threadid1 = 1;

	//Load the map
	MapLoad("moon.fmp");

	//Install digital sound driver
	if (install_sound(DIGI_AUTODETECT, MIDI_NONE, "") != 0) {
		allegro_message("Error initializing sound system"); //Tell user that sound couldn't be initialized
		return 1;
	}
	rectfill(screen, 0, 0, SCREEN_W, SCREEN_H, WHITE); //Colour the screen white

													   //***Declare/Define BITMAPS***
	buffer = create_bitmap(1024, 768); //Buffer screen

	BITMAP *temp; //To hold incoming sprites
	hpHeart = create_bitmap(32, 32); //Create hpHeart bitmap
	tempBuf = create_bitmap(1024, 768);
	rectfill(tempBuf, 0, 0, 1024, 768, makecol(255, 0, 255));
	//****************************

	//***AUDIO***
	//Background song
	volume = 155;
	bg = (SAMPLE*)datFile[0].dat; //Load the bg.wav file, assign to intro
	if (!bg) { //If bg.wav can't be found
		allegro_message("Couldn't find the bg.wav file"); //Tell user. If this happens, make sure files are in proper folder
		return 1;
	}

	//Hurt sound - The rest follow the smae logic
	hurt = (SAMPLE*)datFile[7].dat;
	if (!hurt) {
		allegro_message("Couldn't find the hurt.wav file");
		return 1;
	}
	//Healed sound
	healed = (SAMPLE*)datFile[5].dat;
	if (!healed) {
		allegro_message("Couldn't find the healed.wav file");
		return 1;
	}

	//Collectable sound
	collectable = (SAMPLE*)datFile[3].dat;
	if (!collectable) {
		allegro_message("Couldn't find the collectable.wav file");
		return 1;
	}

	//Allocating bg.wav to soundcard voice
	play_sample(bg, volume, 128, 1000, TRUE);
	//***********

	//***Load Fonts***
	bodyFont = (FONT*)datFile[1].dat; //Font for body text
	titleFont = (FONT*)datFile[10].dat; //Font for title

										//Menu loop
	while (true) {
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Your ship has crashed on the moon") / 2, SCREEN_H / 2, WHITE, -1, "Your ship has crashed on the moon");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "and you have angered its inhabitants... The cheeses.") / 2, SCREEN_H / 2 + 20, WHITE, -1, "and you have angered its inhabitants... The cheeses.");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Don't let them bite you and don't fall into the chasms!") / 2, SCREEN_H / 2 + 40, WHITE, -1, "Don't let them bite you and don't fall into the chasms!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "You need to collect all of your ship parts to get back home!") / 2, SCREEN_H / 2 + 60, WHITE, -1, "You need to collect all of your ship parts to get back home!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(11000);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		release_bitmap(buffer);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, WHITE, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, WHITE, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, WHITE, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump and spacebar again in mid-air to double jump") / 2, SCREEN_H / 2, WHITE, -1, "spacebar to jump and spacebar again in mid-air to double jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, WHITE, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, WHITE, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump and spacebar again in mid-air to double jump") / 2, SCREEN_H / 2, WHITE, -1, "spacebar to jump and spacebar again in mid-air to double jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, WHITE, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, WHITE, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, WHITE, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump and spacebar again in mid-air to double jump") / 2, SCREEN_H / 2, WHITE, -1, "spacebar to jump and spacebar again in mid-air to double jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, WHITE, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, WHITE, -1, "ctrl+h to see controls again");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1200);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, WHITE, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, WHITE, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump and spacebar again in mid-air to double jump") / 2, SCREEN_H / 2, WHITE, -1, "spacebar to jump and spacebar again in mid-air to double jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, WHITE, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, WHITE, -1, "ctrl+h to see controls again");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ESC to quit at any time") / 2, SCREEN_H / 2 + 60, WHITE, -1, "ESC to quit at any time");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(2000);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, WHITE, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, WHITE, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump and spacebar again in mid-air to double jump") / 2, SCREEN_H / 2, WHITE, -1, "spacebar to jump and spacebar again in mid-air to double jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, WHITE, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, WHITE, -1, "ctrl+h to see controls again");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ESC to quit at any time") / 2, SCREEN_H / 2 + 60, WHITE, -1, "ESC to quit at any time");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Are you ready?") / 2, SCREEN_H / 2 + 120, WHITE, -1, "Are you ready?");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		rest(1500);
		acquire_bitmap(buffer);
		rectfill(buffer, 0, 0, 1024, 768, BLACK);
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "The Adventures of") / 2, SCREEN_H / 2 - 150, WHITE, -1, "The adventures of");
		textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Glorp!") / 2, SCREEN_H / 2 - 130, WHITE, -1, "Glorp!");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "CONTROLS") / 2, SCREEN_H / 2 - 40, WHITE, -1, "CONTROLS");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "<- and -> arrow keys for movement") / 2, SCREEN_H / 2 - 20, WHITE, -1, "<- and -> arrow keys for movement");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "spacebar to jump and spacebar again in mid-air to double jump") / 2, SCREEN_H / 2, WHITE, -1, "spacebar to jump and spacebar again in mid-air to double jump");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+m to mute/unmute music") / 2, SCREEN_H / 2 + 20, WHITE, -1, "ctrl+m to mute/unmute music");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ctrl+h to see controls again") / 2, SCREEN_H / 2 + 40, WHITE, -1, "ctrl+h to see controls again");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "ESC to quit at any time") / 2, SCREEN_H / 2 + 60, WHITE, -1, "ESC to quit at any time");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Are you ready?") / 2, SCREEN_H / 2 + 120, WHITE, -1, "Are you ready?");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "press any key to start...") / 2, SCREEN_H / 2 + 140, WHITE, -1, "press any key to start...");
		textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Copyright (c) 2017 Chad Cromwell") / 2, SCREEN_H - 20, WHITE, -1, "Copyright (c) 2017 Chad Cromwell");
		release_bitmap(buffer);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
		clear_bitmap(buffer);
		clear_keybuf();
		readkey();
		break;
	}

	//Load part.bmp
	temp = (BITMAP*)datFile[8].dat;
	for (int n = 0; n < 2; n++) {
		partImg[n] = grabframe(temp, 32, 32, 0, 0, 2, n);
	}
	destroy_bitmap(temp);

	//Load cheese.bmp
	temp = (BITMAP*)datFile[2].dat;
	for (int n = 0; n < 2; n++) {
		cheeseImg[n] = grabframe(temp, 32, 32, 0, 0, 2, n);
	}
	destroy_bitmap(temp);

	//Load heart.bmp
	temp = (BITMAP*)datFile[6].dat;
	heartImg[0] = grabframe(temp, 32, 32, 0, 0, 1, 0);
	blit(temp, hpHeart, 0, 0, 0, 0, 32, 32);

	destroy_bitmap(temp); //Destroy temp bitmap for reuse

						  //Load ship.bmp
	temp = (BITMAP*)datFile[9].dat;
	for (int n = 0; n < 2; n++) {
		shipImg[n] = grabframe(temp, 128, 128, 0, 0, 2, n);
	}

	//Load Glorp
	temp = (BITMAP*)datFile[4].dat;
	for (int n = 0; n < 10; n++) { //Iterate through 20 frames
		glorpImg[n] = grabframe(temp, 32, 32, 0, 0, 10, n); //Grab each frame and new position and assign to element
	}
	for (int n = 10; n < 20; n++) { //Iterate through 20 frames
		glorpImg[n] = grabframe(temp, 32, 32, 0, 0, 10, n); //Grab each frame and new position and assign to element
	}
	destroy_bitmap(temp); //Destroy bitmap for reuse

						  //*******Initialize Sprites***********
						  //Initialize Glorp
	glorp->x = 20;
	glorp->y = HEIGHT - 94;
	glorp->width = 32;
	glorp->height = 32;
	glorp->xdelay = 1;
	glorp->ydelay = 0;
	glorp->xcount = 0;
	glorp->ycount = 0;
	glorp->xVelocity = 0;
	glorp->yVelocity = 0;
	glorp->curframe = 0;
	glorp->maxframe = 19;
	glorp->framecount = 0;
	glorp->framedelay = 2;
	glorp->animdir = 1;
	glorp->dir = 0;
	glorp->facing = 0;
	glorp->hp = 3;

	//Initialize parts
	part->x = 610;
	part->y = 695;
	part->minX = part->x;
	part->minY = part->y;
	part->maxX = 610;
	part->maxY = 705;
	part->width = partImg[0]->w;
	part->height = partImg[0]->h;
	part->xdelay = 0;
	part->ydelay = 0;
	part->xcount = 0;
	part->ycount = 0;
	part->xVelocity = 0;
	part->yVelocity = 0;
	part->curframe = 0;
	part->maxframe = 1;
	part->framecount = 0;
	part->framedelay = 50;
	part->animdir = 1;
	part->dir = 3;
	part->type = "collectable";
	part->alive = true;

	part2->x = 1185;
	part2->y = 695;
	part2->minX = part2->x;
	part2->minY = part2->y;
	part2->maxX = 1185;
	part2->maxY = 705;
	part2->width = partImg[0]->w;
	part2->height = partImg[0]->h;
	part2->xdelay = 0;
	part2->ydelay = 0;
	part2->xcount = 0;
	part2->ycount = 0;
	part2->xVelocity = 0;
	part2->yVelocity = 0;
	part2->curframe = 0;
	part2->maxframe = 1;
	part2->framecount = 0;
	part2->framedelay = 50;
	part2->animdir = 1;
	part2->dir = 3;
	part2->type = "collectable";
	part2->alive = true;

	part3->x = 1990;
	part3->y = 125;
	part3->minX = part3->x;
	part3->minY = part3->y;
	part3->maxX = 1990;
	part3->maxY = 135;
	part3->width = partImg[0]->w;
	part3->height = partImg[0]->h;
	part3->xdelay = 0;
	part3->ydelay = 0;
	part3->xcount = 0;
	part3->ycount = 0;
	part3->xVelocity = 0;
	part3->yVelocity = 0;
	part3->curframe = 0;
	part3->maxframe = 1;
	part3->framecount = 0;
	part3->framedelay = 50;
	part3->animdir = 1;
	part3->dir = 3;
	part3->type = "collectable";
	part3->alive = true;

	//Initialize Evil Cheese
	cheese->x = 610;
	cheese->y = 705;
	cheese->minX = cheese->x;
	cheese->minY = cheese->y;
	cheese->maxX = 735;
	cheese->maxY = 705;
	cheese->width = cheeseImg[0]->w;
	cheese->height = cheeseImg[0]->h;
	cheese->xdelay = 0;
	cheese->ydelay = 0;
	cheese->xcount = 0;
	cheese->ycount = 0;
	cheese->xVelocity = 0;
	cheese->yVelocity = 0;
	cheese->curframe = 0;
	cheese->maxframe = 1;
	cheese->framecount = 0;
	cheese->framedelay = 10;
	cheese->animdir = 1;
	cheese->dir = 1;
	cheese->type = "hurt";
	cheese->alive = true;

	cheese2->x = 705;
	cheese2->y = 400;
	cheese2->minX = cheese2->x;
	cheese2->minY = cheese2->y;
	cheese2->maxX = 705;
	cheese2->maxY = 540;
	cheese2->width = cheeseImg[0]->w;
	cheese2->height = cheeseImg[0]->h;
	cheese2->xdelay = 0;
	cheese2->ydelay = 0;
	cheese2->xcount = 0;
	cheese2->ycount = 0;
	cheese2->xVelocity = 0;
	cheese2->yVelocity = 0;
	cheese2->curframe = 0;
	cheese2->maxframe = 1;
	cheese2->framecount = 0;
	cheese2->framedelay = 10;
	cheese2->animdir = 1;
	cheese2->dir = 3;
	cheese2->type = "hurt";
	cheese2->alive = true;

	cheese3->x = 932;
	cheese3->y = 705;
	cheese3->minX = cheese3->x;
	cheese3->minY = cheese3->y;
	cheese3->maxX = 1185;
	cheese3->maxY = 705;
	cheese3->width = cheeseImg[0]->w;
	cheese3->height = cheeseImg[0]->h;
	cheese3->xdelay = 0;
	cheese3->ydelay = 0;
	cheese3->xcount = 0;
	cheese3->ycount = 0;
	cheese3->xVelocity = 0;
	cheese3->yVelocity = 0;
	cheese3->curframe = 0;
	cheese3->maxframe = 1;
	cheese3->framecount = 0;
	cheese3->framedelay = 10;
	cheese3->animdir = 1;
	cheese3->dir = 1;
	cheese3->type = "hurt";
	cheese3->alive = true;

	cheese4->x = 1345;
	cheese4->y = 210;
	cheese4->minX = cheese4->x;
	cheese4->minY = cheese4->y;
	cheese4->maxX = 1345;
	cheese4->maxY = 410;
	cheese4->width = cheeseImg[0]->w;
	cheese4->height = cheeseImg[0]->h;
	cheese4->xdelay = 0;
	cheese4->ydelay = 0;
	cheese4->xcount = 0;
	cheese4->ycount = 0;
	cheese4->xVelocity = 0;
	cheese4->yVelocity = 0;
	cheese4->curframe = 0;
	cheese4->maxframe = 1;
	cheese4->framecount = 0;
	cheese4->framedelay = 10;
	cheese4->animdir = 1;
	cheese4->dir = 3;
	cheese4->type = "hurt";
	cheese4->alive = true;

	cheese5->x = 1345;
	cheese5->y = 705;
	cheese5->minX = cheese5->x;
	cheese5->minY = cheese5->y;
	cheese5->maxX = 1920;
	cheese5->maxY = 705;
	cheese5->width = cheeseImg[0]->w;
	cheese5->height = cheeseImg[0]->h;
	cheese5->xdelay = 0;
	cheese5->ydelay = 0;
	cheese5->xcount = 0;
	cheese5->ycount = 0;
	cheese5->xVelocity = 0;
	cheese5->yVelocity = 0;
	cheese5->curframe = 0;
	cheese5->maxframe = 1;
	cheese5->framecount = 0;
	cheese5->framedelay = 10;
	cheese5->animdir = 1;
	cheese5->dir = 1;
	cheese5->type = "hurt";
	cheese5->alive = true;

	cheese6->x = 2280;
	cheese6->y = 100;
	cheese6->minX = cheese6->x;
	cheese6->minY = cheese6->y;
	cheese6->maxX = 2280;
	cheese6->maxY = 510;
	cheese6->width = cheeseImg[0]->w;
	cheese6->height = cheeseImg[0]->h;
	cheese6->xdelay = 0;
	cheese6->ydelay = 0;
	cheese6->xcount = 0;
	cheese6->ycount = 0;
	cheese6->xVelocity = 0;
	cheese6->yVelocity = 0;
	cheese6->curframe = 0;
	cheese6->maxframe = 1;
	cheese6->framecount = 0;
	cheese6->framedelay = 10;
	cheese6->animdir = 1;
	cheese6->dir = 3;
	cheese6->type = "hurt";
	cheese6->alive = true;

	cheese7->x = 2480;
	cheese7->y = 100;
	cheese7->minX = cheese7->x;
	cheese7->minY = cheese7->y;
	cheese7->maxX = 2480;
	cheese7->maxY = 410;
	cheese7->width = cheeseImg[0]->w;
	cheese7->height = cheeseImg[0]->h;
	cheese7->xdelay = 0;
	cheese7->ydelay = 0;
	cheese7->xcount = 0;
	cheese7->ycount = 0;
	cheese7->xVelocity = 0;
	cheese7->yVelocity = 0;
	cheese7->curframe = 0;
	cheese7->maxframe = 1;
	cheese7->framecount = 0;
	cheese7->framedelay = 10;
	cheese7->animdir = 1;
	cheese7->dir = 3;
	cheese7->type = "hurt";
	cheese7->alive = true;

	cheese8->x = 3298;
	cheese8->y = 705;
	cheese8->minX = cheese8->x;
	cheese8->minY = cheese8->y;
	cheese8->maxX = 3430;
	cheese8->maxY = 705;
	cheese8->width = cheeseImg[0]->w;
	cheese8->height = cheeseImg[0]->h;
	cheese8->xdelay = 0;
	cheese8->ydelay = 0;
	cheese8->xcount = 0;
	cheese8->ycount = 0;
	cheese8->xVelocity = 0;
	cheese8->yVelocity = 0;
	cheese8->curframe = 0;
	cheese8->maxframe = 1;
	cheese8->framecount = 0;
	cheese8->framedelay = 10;
	cheese8->animdir = 1;
	cheese8->dir = 1;
	cheese8->type = "hurt";
	cheese8->alive = true;

	cheese9->x = 3490;
	cheese9->y = 705;
	cheese9->minX = cheese9->x;
	cheese9->minY = cheese9->y;
	cheese9->maxX = 3588;
	cheese9->maxY = 705;
	cheese9->width = cheeseImg[0]->w;
	cheese9->height = cheeseImg[0]->h;
	cheese9->xdelay = 0;
	cheese9->ydelay = 0;
	cheese9->xcount = 0;
	cheese9->ycount = 0;
	cheese9->xVelocity = 0;
	cheese9->yVelocity = 0;
	cheese9->curframe = 0;
	cheese9->maxframe = 1;
	cheese9->framecount = 0;
	cheese9->framedelay = 10;
	cheese9->animdir = 1;
	cheese9->dir = 1;
	cheese9->type = "hurt";
	cheese9->alive = true;

	cheese10->x = 3652;
	cheese10->y = 705;
	cheese10->minX = cheese10->x;
	cheese10->minY = cheese10->y;
	cheese10->maxX = 3748;
	cheese10->maxY = 705;
	cheese10->width = cheeseImg[0]->w;
	cheese10->height = cheeseImg[0]->h;
	cheese10->xdelay = 0;
	cheese10->ydelay = 0;
	cheese10->xcount = 0;
	cheese10->ycount = 0;
	cheese10->xVelocity = 0;
	cheese10->yVelocity = 0;
	cheese10->curframe = 0;
	cheese10->maxframe = 1;
	cheese10->framecount = 0;
	cheese10->framedelay = 10;
	cheese10->animdir = 1;
	cheese10->dir = 1;
	cheese10->type = "hurt";
	cheese10->alive = true;

	//Initialize heart
	heart->x = 1345;
	heart->y = 642;
	heart->minX = heart->x;
	heart->minY = heart->y;
	heart->maxX = 1345;
	heart->maxY = 690;
	heart->width = heartImg[0]->w;
	heart->height = heartImg[0]->h;
	heart->xdelay = 0;
	heart->ydelay = 0;
	heart->xcount = 0;
	heart->ycount = 0;
	heart->xVelocity = 0;
	heart->yVelocity = 0;
	heart->curframe = 0;
	heart->maxframe = 0;
	heart->framecount = 0;
	heart->framedelay = 1;
	heart->animdir = 1;
	heart->dir = 3; //Start the heart going vertical
	heart->type = "heart";
	heart->alive = true;

	heart2->x = 3539;
	heart2->y = 500;
	heart2->minX = heart2->x;
	heart2->minY = heart2->y;
	heart2->maxX = 3539;
	heart2->maxY = 550;
	heart2->width = heartImg[0]->w;
	heart2->height = heartImg[0]->h;
	heart2->xdelay = 0;
	heart2->ydelay = 0;
	heart2->xcount = 0;
	heart2->ycount = 0;
	heart2->xVelocity = 0;
	heart2->yVelocity = 0;
	heart2->curframe = 0;
	heart2->maxframe = 0;
	heart2->framecount = 0;
	heart2->framedelay = 1;
	heart2->animdir = 1;
	heart2->dir = 3; //Start heart2 going vertical
	heart2->type = "heart";
	heart2->alive = true;

	//Initialize Ship
	ship->x = (mapwidth*mapblockwidth) - shipImg[0]->w;
	ship->y = 625;
	ship->minX = ship->x;
	ship->minY = ship->y;
	ship->maxX = (mapwidth*mapblockwidth) - shipImg[0]->w;
	ship->maxY = 625;
	ship->width = shipImg[0]->w;
	ship->height = shipImg[0]->h;
	ship->xdelay = 0;
	ship->ydelay = 0;
	ship->xcount = 0;
	ship->ycount = 0;
	ship->xVelocity = 0;
	ship->yVelocity = 0;
	ship->curframe = 0;
	ship->maxframe = 1;
	ship->framecount = 0;
	ship->framedelay = 30;
	ship->animdir = 1;
	ship->dir = 1;
	ship->type = "end";
	ship->alive = true;
	//************************************

	//***Set Framerate***
	LOCK_VARIABLE(counter);
	LOCK_FUNCTION(Increment);
	install_int_ex(Increment, BPS_TO_TIMER(60)); //60FPS
												 //*******************

	id = pthread_create(&pthread0, NULL, thread0, (void*)&threadid0);
	//id = pthread_create(&pthread1, NULL, thread1, (void*)&threadid1);

	//***Game Loop***
	while (gameOver == false) { //If gameOver is false
		while (dead == true) { //If player is dead display dead screen
			if (keypressed()) { //If key is pressed
				acquire_bitmap(buffer);
				rectfill(buffer, 0, 0, 1024, 768, BLACK);
				textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "You Died!") / 2, SCREEN_H / 2 - 80, WHITE, -1, "You Died!");
				textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Press ESC to quit") / 2, SCREEN_H / 2 + 20, WHITE, -1, "Press ESC to quit");
				release_bitmap(buffer);
				blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
				//If user presses ESC, quit game
				if (key[KEY_ESC]) {
					release_bitmap(buffer); //Release the bitmap or else it will crash, because the game is still drawing in the background
					clear_bitmap(buffer); //Clear the bitmap
					gameOver = true; //Game over true, exit game loop
					break; //Break out of dead loop
				}
			}
		}

		//Score screen, same logic as above
		while (scoreScreen == true) {
			if (keypressed()) {
				acquire_bitmap(buffer);
				rectfill(buffer, 0, 0, 1024, 768, BLACK);
				textprintf_ex(buffer, titleFont, SCREEN_W / 2 - text_length(titleFont, "Level Complete!") / 2, SCREEN_H / 2 - 80, WHITE, -1, "Level Complete!");
				textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "You found #/3 ship parts!") / 2, SCREEN_H / 2, WHITE, -1, "You found %i/3 ship parts!", shipParts); //Print to screen how much time has passed
				textprintf_ex(buffer, bodyFont, SCREEN_W / 2 - text_length(bodyFont, "Press ESC to quit") / 2, SCREEN_H / 2 + 40, WHITE, -1, "Press ESC to quit");
				release_bitmap(buffer);
				blit(buffer, screen, 0, 0, 0, 0, 1024, 768);
				if (key[KEY_ESC]) {
					release_bitmap(buffer);
					clear_bitmap(buffer);
					gameOver = true;
					break;
				}
			}
		}

		//When counter reaches 0, wait for one count
		while (counter == 0) {
			rest(1);
		}

		//While counter is greater than 0
		while (counter > 0) {
			pthread_mutex_lock(&threadsafe);
			//Assign the counter number to old_counter
			int old_counter = counter;

			//Draw the background/ground
			drawLevel(buffer);

			//Apply physics
			physics(glorp);

			//Move Sprite Guy
			controller(glorp);

			//Draw Sprite Guy to buffer
			acquire_bitmap(buffer);

			//Animate sprites
			animate(buffer, glorp); //Animate glorp

									//Hearts
			spawnPacer(buffer, heartImg, heart, .01, 1);
			spawnPacer(buffer, heartImg, heart2, .01, 1);

			//Parts
			spawnPacer(buffer, partImg, part, .01, 1);
			spawnPacer(buffer, partImg, part2, .01, 1);
			spawnPacer(buffer, partImg, part3, .01, 1);

			//Cheeses
			spawnHomingPacer(buffer, cheeseImg, cheese, .08, 2);
			spawnHomingPacer(buffer, cheeseImg, cheese2, .08, 2);
			spawnPacer(buffer, cheeseImg, cheese3, .08, 4);
			spawnPacer(buffer, cheeseImg, cheese4, .08, 3);
			spawnPacer(buffer, cheeseImg, cheese5, .08, 6);
			spawnHomingPacer(buffer, cheeseImg, cheese6, .08, 6);
			spawnPacer(buffer, cheeseImg, cheese7, .08, 5);
			spawnHomingPacer(buffer, cheeseImg, cheese8, .08, 3);
			spawnPacer(buffer, cheeseImg, cheese9, .08, 4);
			spawnPacer(buffer, cheeseImg, cheese10, .08, 2);

			//Ship
			spawnPacer(buffer, shipImg, ship, 0, 0);

			pthread_mutex_unlock(&threadsafe);

			//When CTRL+H is pressed - Display controls
			if (key[KEY_LCONTROL] && key[KEY_H] || key[KEY_RCONTROL] && key[KEY_H]) {
				rectfill(buffer, 0, SCREEN_H - 20, text_length(font, "<- -> LEFT and RIGHT ARROW KEYS to MOVE. SPACEBAR to JUMP. CTRL-M to MUTE music. CTRL-H to show this HELP window. ESC to quit."), SCREEN_H, WHITE);
				textprintf_ex(buffer, font, 0, SCREEN_H - 15, BLACK, WHITE, "<- -> LEFT and RIGHT ARROW KEYS to MOVE. SPACEBAR to JUMP. CTRL-M to MUTE music. CTRL-H to show this HELP window. ESC to quit.");
			}
			release_bitmap(buffer);

			//Increment ticks for time count
			ticks++;

			//Decrement counter
			counter--;

			//If frame is taking too long to compute, break out of the frame and just draw
			if (old_counter <= counter) {
				break;
			}
		}

		masked_blit(tempBuf, buffer, 0, 0, 0, 0, 1024, 768);
		blit(buffer, screen, 0, 0, 0, 0, 1024, 768); //Blit the buffer to the screen
		clear_bitmap(buffer); //Clear the buffer for next frame
		rectfill(tempBuf, 0, 0, 1024, 768, makecol(255, 0, 255));

	}
	//***************
	//Destroy SPRITES and BITMAPS
	for (int n = 0; n < 20; n++) {
		destroy_bitmap(glorpImg[n]);
	}
	pthread_mutex_destroy(&threadsafe);
	destroy_bitmap(heartImg[0]);
	destroy_bitmap(partImg[0]);
	destroy_bitmap(cheeseImg[0]);
	destroy_bitmap(buffer);
	destroy_bitmap(hpHeart);
	destroy_sample(bg);
	destroy_sample(healed);
	destroy_sample(hurt);
	MapFreeMem();
	allegro_exit();
	return 0;
}

END_OF_MAIN();