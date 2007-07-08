#include <pspkernel.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "../../triAt3.h"
#include "../../triFont.h"
#include "../../triTimer.h"
#include "../../triInput.h"
#include "../../triImage.h"
#include "../../triGraphics.h"
#include "../../triTypes.h"
#include "../../triLog.h"
#include "../../triMemory.h"
#include "../../triWav.h"
#ifdef DEBUG_CONSOLE
#include "../../triConsole.h"
#endif

//#define NOSPLASHES 0
#define RELEASE

PSP_MODULE_INFO("Mudkip Adventures", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU|PSP_THREAD_ATTR_USER);

static int isrunning = 1;

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	isrunning = 0;
	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread,
				     0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}


/*
#define MINICHARSET "ABCDEFGHIJKLM12345NOPQRSTUVWXYZ67890"
#define CHARXOFFS(x) ((n%18)*7)
#define CHARYOFFS(x) ((n/18)*10)
*/

#define STATUSSHADOW 0xff9fb6b6
#define STATUSCOLOR 0xff404040


#define SIZE_10 0
#define SIZE_12 1
#define SIZE_14 2


static triFont* hollow[3] = { 0 };
static triFont* solid[3] = { 0 };
static triFont* verdana10 = 0;


#ifdef DEBUG_CONSOLE
CVARF( debugcollisionmap, 1.0 )
CVARF( unlimitedaqua, 0.0 )

void spawnenemy( triS32 type, triS32 minlevel, triS32 maxlevel, triS32 dx, triS32 dy );
void qlevelup();

triChar* cmd_levelup()
{
	triS32 nlevels = 1;
	if (triCmdArgc()>1)
		nlevels = atoi(triCmdArgv(1));
	if (nlevels<1) nlevels = 1;
	if (nlevels>98) nlevels = 98;
	while (nlevels-->0)
		qlevelup();
	return(0);
}

triChar* cmd_spawn()
{
	spawnenemy( rand()%4, 1, 5, 15, 9 );
	return(0);
}
#endif

void init()
{
	SetupCallbacks();
	triInit(GU_PSM_8888);
	triInputInit();
	triMemoryInit();
	triLogInit();
	triFontInit();
	triWavInit();
	triAt3Init();
	
	triSInt sz = 10;
	triSInt tex = 128;
	triSInt i = 0;
	for (;i<3;i++,sz+=2)
	{
		//hollow[i] = triFontLoad( "data/hollow.ttf", sz, TRI_FONT_SIZE_PIXELS, tex, TRI_FONT_VRAM );
		solid[i] = triFontLoad( "data/solid.ttf", sz, TRI_FONT_SIZE_PIXELS, tex, TRI_FONT_VRAM );
		if (i==1) tex <<= 1;
	}
	
	verdana10 = triFontLoad( "data/verdana.ttf", 10, TRI_FONT_SIZE_PIXELS, 128, TRI_FONT_VRAM );
	
	#ifdef DEBUG_CONSOLE
	triLogPrint("Initiating console...\n");
	triConsoleInit();
	triCVarRegister( &debugcollisionmap );
	triCVarRegister( &unlimitedaqua );
	triCmdRegister( "spawn", cmd_spawn, 0, 0, "Spawn a new enemy", 0 );
	triCmdRegister( "levelup", cmd_levelup, 0, 0, "One level up", 0 );
	#endif
}

void deinit()
{
	#ifdef DEBUG_CONSOLE
	triConsoleClose();
	#endif
	triSInt i = 0;
	for (;i<3;i++)
	{
		triFontUnload( solid[i] );
	}
	triFontUnload( verdana10 );
	triClose();
	triInputShutdown();
	triFontShutdown();
	triMemoryShutdown();
	triLogShutdown();
}


void fadetocol( triU32 col, triFloat time )
{
	triChar* temp = triMalloc( FRAME_BUFFER_SIZE*triBpp );
	if (temp==0) return;
	sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, triFramebuffer, 0, 0, 512, temp );
	triImageBlend( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 );
	triTimer* timer = triTimerCreate();
	if (timer==0) return;
	triFloat dt;
	while((dt=triTimerPeekDeltaTime(timer))<time)
	{
		dt = 255.0f * dt / time;
		sceGuTexSync();
		triDrawRect( 0, 0, 480, 272, (col&0xFFFFFF)|((triU32)(dt)<<24) );
		triSwapbuffers();
		sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, temp, 0, 0, 512, triFramebuffer );
	}
	triDrawRect( 0, 0, 480, 272, col | (0xFF<<24) );
	triTimerFree( timer );
	triFree( temp );
	triSwapbuffers();
}


void fadefromcol( triU32 col, triFloat time )
{
	triChar* temp = triMalloc( FRAME_BUFFER_SIZE*triBpp );
	if (temp==0) return;
	sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, triFramebuffer, 0, 0, 512, temp );
	triImageBlend( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 );
	triTimer* timer = triTimerCreate();
	if (timer==0) return;
	triFloat dt;
	while((dt=triTimerPeekDeltaTime(timer))<time)
	{
		dt = 255.0f - 255.0f * dt / time;
		sceGuTexSync();
		triDrawRect( 0, 0, 480, 272, (col&0xFFFFFF)|((triU32)(dt)<<24) );
		triSwapbuffers();
		sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, temp, 0, 0, 512, triFramebuffer );
	}
	sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, temp, 0, 0, 512, triFramebuffer );
	triTimerFree( timer );
	triFree( temp );
	sceGuTexSync();
	triSwapbuffers();
}


triImage* msgborder = 0;

triS32 drawmsgbox( triS32 x, triS32 y, triS32 width, triS32 lines, triFont* font, triChar* msg )
{
	triS32 i;
	
	triS32 height = 24+(font->fontHeight+3)*lines;

	triS32 printed = 0;
	sceGuTexFunc( GU_TFX_REPLACE, GU_TCC_RGBA );
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	triDrawImage( x, y, 8, 8, 0, 0, 8, 8, msgborder );
	triDrawImage( x+8, y, (width-16), 8,  9, 0, 17, 8, msgborder );
	triDrawImage( x+width-8, y, 8, 8, 18, 0, 26, 8, msgborder );
	triDrawImage( x, y+8, 8, height-16, 0, 9, 8, 17, msgborder );
	triDrawImage( x+8, y+8, width-16, height-16, 9, 9, 17, 17, msgborder );
	triDrawImage( x+width-8, y+8, 8, height-16, 18, 9, 26, 17, msgborder );
	triDrawImage( x, y+height-8, 8, 8, 0, 18, 8, 26, msgborder );
	triDrawImage( x+8, y+height-8, width-16, 8, 9, 18, 17, 26, msgborder );
	triDrawImage( x+width-8, y+height-8, 8, 8, 18, 18, 26, 26, msgborder );

	triFontActivate( font );
	triS32 ln = 0;
	while (ln<lines)
	{
		triS32 lastword = 0;
		triS32 lastchar = 0;
		triS32 txtwidth = 0;
		triChar chr[2] = {0};
		i = 0;
		while(txtwidth<(width-24))
		{
			if (msg[lastchar]=='\0')
				break;
			if (msg[lastchar]=='\n')
			{
				lastchar++;
				break;
			}
			if (msg[lastchar]==' ' || msg[lastchar]=='-')
				lastword = lastchar+1;
			chr[0] = msg[lastchar++];
			txtwidth += triFontMeasureText( font, chr );
		}
		if (txtwidth>=(width-24))
		{
			lastchar--;
			if (lastword!=0)
				lastchar = lastword;	// cut at words
		}
		triChar temp[128];
		strncpy( temp, msg, lastchar );
		temp[lastchar] = '\0';
		msg += lastchar;
		printed += lastchar;
		triFontPrint( font, x + 13, y + ln*(font->fontHeight+3) + 13, 0xFF000000, temp );
		triFontPrint( font, x + 12, y + ln*(font->fontHeight+3) + 12, 0x7FFFFFFF, temp );
		ln++;
	}
	
	sceGuTexFunc( GU_TFX_REPLACE, GU_TCC_RGBA );
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	return printed;
}

void msgbox( triS32 x, triS32 y, triS32 width, triS32 lines, triFont* font, triS32 ctrl, triChar* msg )
{
	triS32 height = 24+(font->fontHeight+3)*lines;
	triChar* temp = triMalloc( FRAME_BUFFER_SIZE*triBpp );
	sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, triFramebuffer, 0, 0, 512, temp );
	
	triCopyFromScreen( 0, 0, 480, 272, 0, 0, 512, triFramebuffer );
	triS32 firstframe = 1;
	if (ctrl==0) ctrl = PSP_CTRL_CROSS;
	while (msg!=0 && msg[0]!=0)
	{
		msg += drawmsgbox( x, y, width, lines, font, msg );
		/* TODO: Animate the arrow */
		if (msg[0]!=0)
			triDrawImage( x+width-17, y+12, 10, 7,  16, 27, 26, 34, msgborder );
		
		if (ctrl&PSP_CTRL_CROSS)
			triDrawImage( x+width-14, y+height-14, 16, 16, 0, 26, 16, 41, msgborder );
		else
		if (ctrl&PSP_CTRL_START)
			triDrawImage( x+width-22, y+height-12, 26, 14, 0, 42, 26, 56, msgborder );
		else
		if (ctrl&PSP_CTRL_SELECT)
			triDrawImage( x+width-22, y+height-12, 26, 14, 0, 57, 26, 71, msgborder );
		
		triSwapbuffers();
		if (firstframe)
		{
			sceKernelDelayThread(250*1000);
			firstframe = 0;
		}
		triInputUpdate();
		while (!triInputPressed(ctrl))
		{
			triInputUpdate();
			sceKernelDelayThread(100*1000);
		}
	}
	sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, temp, 0, 0, 512, triFramebuffer );
	triSwapbuffers();
	sceGuCopyImage( triPsm, 0, 0, 480, 272, 512, temp, 0, 0, 512, triFramebuffer );
	triFree( temp );
	sceKernelDelayThread(250*1000);
}


// playfield map
triU8*	pmap = 0;
triS32	pwidth = 0;
triS32	pheight = 0;


#define DIR_DOWN 0
#define DIR_UP 1
#define DIR_LEFT 2
#define DIR_RIGHT 3



#define ATTACK_TACKLE 1
#define ATTACK_BUBBLE 2
#define ATTACK_SLAP 4
#define ATTACK_WATERGUN 8
#define ATTACK_WHIRLPOOL 16
#define ATTACK_HYDROPUMP 32

struct
{
	triImage*			spritesheet;
	triImageAnimation*	ani[4];
	triS32				level;
	triFloat			hp;
	triFloat			ap;
	triS32				attacks;		// Bitmask of available attacks
	triS32				berries;
	triS32				exp;
	triS32				hitenemies;
	triS32				berriessaved;
	triS32				direction;
	triS32				tackle;			// Is doing a tackle
	triS32				speed;
	triFloat			x, y;
	triFloat			lx, ly;			// Last x,y
	triTimer*			timer;
} player;



triS32	isgameover = 0;

void gameover()
{
	isgameover = 1;
	triChar msg[512];
	snprintf( msg, 512, "GAME OVER!\nYou reached up to Level %i. Congratulations!\nHit enemies: %i\nSaved berries: %i", player.level, player.hitenemies, player.berriessaved );
	msgbox( 120, (272-50)/2, 240, 2, solid[SIZE_10], PSP_CTRL_START, msg );
}


typedef struct
{
	triImage*			spritesheet;
	triChar*			image;
	triFloat			speed;
	triFloat			hp;
} enemytype;

#define MAX_TYPES 2
enemytype enemytypes[MAX_TYPES] = {	//           HP  AT  DF  SP
		{ 0, "data/enemy1.png", 1.0f, 2.0f },		// Caterpie: 45  30	 35	 45
		{ 0, "data/enemy2.png", 2.0f, 1.5f }		// Rattata:  30	 56  35	 72
		};

typedef struct
{
	triImageAnimation*	ani[4];
	triS32				type;
	triS32				level;
	triS32				direction;
	triFloat			hp;
	triS32				freeze;			// Freeze time through bubble
	triS32				wait;			// Wait time before grabbing berry/getting out of screen
	triS32				run;			// If enemy is running away
	triS32				gotberry;
	triS32				hit;			// Enemy got hit and is about to be thrown back
	triFloat			force;
	triFloat			speed;
	triFloat			x, y;
	triFloat			vx, vy;			// velocity x,y
	triFloat			lx, ly;			// last x,y
	triFloat			dx, dy;			// destination x,y
	triTimer*			timer;
} enemy;


#define TARGET_REACH 5.5f

#define MAP_FREE	0
#define MAP_UNMOVEABLE	1
#define MAP_PLAYER	2
#define MAP_ENEMIES 5

#define MAX_ENEMIES (256-MAP_ENEMIES)
enemy enemies[MAX_ENEMIES];
triS32 enemies_stack[MAX_ENEMIES];
triS32 enemies_idx = 0;
triS32 enemies_active = 0;

void initenemies()
{
	triS32 i;
	for (i=0;i<MAX_TYPES;i++)
	{
		//snprintf( enemytypes[i].image, 128, "data/enemy%i.png", i+1 );
		triLogPrint("%s\n", enemytypes[i].image );
		enemytypes[i].spritesheet = triImageLoad( enemytypes[i].image, TRI_VRAM|TRI_SWIZZLE );
		//enemytypes[i].hp = 2.0f;
		//enemytypes[i].speed = 1.0f+i;
	}
	for (i=0;i<MAX_ENEMIES;i++)
	{
		enemies_stack[i] = i;
	}
}

void deinitenemies()
{
	triLogPrint("Here\r\n");
	triS32 i;
	for (i=0;i<enemies_idx;i++)
	{
		triS32 j;
		for (j=0;j<4;j++)
		{
			triImageAnimation* img = &enemies[enemies_stack[i]].ani[j];
			//triImageAnimationFree( enemies[enemies_stack[i]].ani[j] );
			if (img->palette!=0) triFree(img->palette);
			triImageList *next = 0;
			while (img->frames!=0)
			{
				next = img->frames->next;
				triFree( img->frames );
				img->frames = next;
			}
			triFree(img);
		}
		triTimerFree( enemies[enemies_stack[i]].timer );
	}
	triLogPrint("Here2\r\n");
	for (i=0;i<MAX_TYPES;i++)
		triImageFree( enemytypes[i].spritesheet );
}


void spawnenemy( triS32 type, triS32 minlevel, triS32 maxlevel, triS32 dx, triS32 dy )
{
	if (enemies_idx>=MAX_ENEMIES || enemies_idx>(triS32)(2.0f*sqrtf(player.level)))
		return;
	
	enemy* e = &enemies[enemies_stack[enemies_idx]];
	
	e->type = type;
	e->level = rand()%(maxlevel-minlevel) + minlevel;
	e->hp = enemytypes[type].hp + e->level*0.1f*enemytypes[type].hp;
	e->freeze = 0;
	e->wait = 0;
	e->speed = (enemytypes[type].speed + e->level*0.1f*enemytypes[type].speed)*0.5f;
	e->run = 0;
	e->gotberry = 0;
	e->dx = dx;
	e->dy = dy;

	triS32 j;
	for (j=0;j<4;j++)
	{
		e->ani[j] = triImageAnimationFromSheet2( enemytypes[type].spritesheet, 0, j*24, 32, 24, 3, 1, 1000/(e->speed*5.f) );
		if (e->ani[j]==0)
		{
			triLogPrint("Error loading enemy sprites!\n");
			return;
		}
		e->ani[j]->loops = 1;
	}
	e->direction = rand()%14;
	if (e->direction>0)
	{
		if (e->direction<=3)
			e->direction = DIR_DOWN;
		else
		if (e->direction<=8)
			e->direction = DIR_LEFT;
		else
			e->direction = DIR_RIGHT;
	}
	
	switch (e->direction)
	{
		case DIR_UP:
			e->x = rand()%pwidth;
			e->y = pheight-1;
			e->vx = 0.f;
			e->vy = -1.f;
			break;
		case DIR_DOWN:
			e->x = rand()%pwidth;
			e->y = 0;
			e->vx = 0.f;
			e->vy = 1.f;
			break;
		case DIR_LEFT:
			e->x = pwidth-1;
			e->y = rand()%pheight;
			e->vx = -1.f;
			e->vy = 0.f;
			break;
		case DIR_RIGHT:
			e->x = 0;
			e->y = rand()%pheight;
			e->vx = 1.f;
			e->vy = 0.f;
			break;
	}
	
	while (pmap[(triS32)e->y*pwidth + (triS32)e->x]!=0)
	{
		e->y += e->vy;
		e->x += e->vx;
		if ((triS32)e->y<0)
		{
			e->y = pheight-1;
			e->x = rand()%pwidth;
		}
		else
		if ((triS32)e->y>=pheight)
		{
			e->y = 0;
			e->x = rand()%pheight;
		}
		else
		if ((triS32)e->x<0)
		{
			e->x = pwidth-1;
			e->y = rand()%pheight;
		}
		else
		if ((triS32)e->x>=pwidth)
		{
			e->x = 0;
			e->y = rand()%pheight;
		}
	}
	pmap[(triS32)e->y*pwidth+(triS32)e->x]=MAP_ENEMIES+enemies_idx;
	e->lx = e->x;
	e->ly = e->y;
	e->timer = triTimerCreate();
	enemies_idx++;
	enemies_active++;
}

void removeenemy( triS32 idx )
{
	if (enemies_idx<=0) return;
	enemies_idx--;
	enemies_active--;
	triS32 j;
	for (j=0;j<4;j++)
		triImageAnimationFree( enemies[enemies_stack[idx]].ani[j] );
	
	triTimerFree( enemies[enemies_stack[idx]].timer );
	triS32 temp = enemies_stack[enemies_idx];
	enemies_stack[enemies_idx] = enemies_stack[idx];
	enemies_stack[idx] = temp;
}

void hitenemy( triS32 idx, triFloat vx, triFloat vy, triFloat force, triS32 type )
{
	enemy* e = &enemies[enemies_stack[idx]];
	if (type==ATTACK_BUBBLE)
	{
		if (rand()%100<33)
			e->freeze++;
		if (e->freeze>3) e->freeze = 3;
	}
	else
	if (type==ATTACK_WATERGUN)
	{
		e->force = force;
		e->vx = vx;
		e->vy = vy;
	}
	else
	if (type==ATTACK_TACKLE)
	{
		e->force = force;
		e->vx = vx;
		e->vy = vy;
		if (e->gotberry)
		{
			if (rand()%100<66)
			{
				e->gotberry = 0;
				player.berries++;
				player.berriessaved++;
			}
		}
	}
	e->hp -= force*0.1f;
	
	if (e->hp<=0.f)
	{
		pmap[(triS32)e->y*pwidth + (triS32)e->x] = MAP_FREE;
		removeenemy( idx );
		player.exp += e->level;
		player.hitenemies++;
		if (e->gotberry)
		{
			e->gotberry = 0;
			player.berries++;
			player.berriessaved++;
		}
	}
}

// An enemy reached the target!
void reachedtarget()
{
	player.berries--;
}

triS32 berriesinplay()
{
	triS32 berries = player.berries;
	triS32 i = 0;
	while(i<enemies_idx)
	{
		enemy* e = &enemies[enemies_stack[i++]];
		berries += e->gotberry;
	}
	
	return berries;
}

void updateenemies( triFloat dt )
{
	triS32 i = 0;
	while(i<enemies_idx)
	{
		enemy* e = &enemies[enemies_stack[i]];
		triFloat lerp = triTimerPeekDeltaTime( e->timer );
		if(triImageAnimationIsDone( e->ani[e->direction] ))
			triImageAnimationReset( e->ani[e->direction] );
		if (lerp>=(1.0f/e->speed))
		{
			triTimerUpdate( e->timer );
			e->lx = e->x;
			e->ly = e->y;
			
			triS32 dir = rand()%4;
			triS32 lastdir = dir;
			triFloat lastdist = ((triS32)e->x-e->dx)*((triS32)e->x-e->dx) + ((triS32)e->y-e->dy)*((triS32)e->y-e->dy);
			
			if (e->freeze>0)
			{
				e->freeze--;
				/*if (e->freeze==0 && lastdist<TARGET_REACH)
				{
					reachedtarget();
					e->run = 1;
				}*/
				i++;
				continue;
			}
			
			while (1)
			{
				e->x = e->lx;
				e->y = e->ly;
				if (dir==DIR_UP)
					e->y -= 1.0f;
				else
				if (dir==DIR_DOWN)
					e->y += 1.0f;
				else
				if (dir==DIR_LEFT)
					e->x -= 1.0f;
				else
					e->x += 1.0f;
				
				triFloat dist = (e->x-e->dx)*(e->x-e->dx) + (e->y-e->dy)*(e->y-e->dy);
				if (e->run==1 && dist>lastdist && (e->x<0 || e->x>=pwidth || e->y<0 || e->y>=pheight || pmap[(triS32)e->y*pwidth+(triS32)e->x]==MAP_UNMOVEABLE))
				{
					removeenemy( i );
					// Enemy got away!
					break;
				}

				/*if (e->x<0) e->x = 0.f;
				if (e->x>=pwidth) e->x = pwidth-1;
				if (e->y<0) e->y = 0.f;
				if (e->y>=pheight) e->y = pheight-1;*/
				dist = (e->x-e->dx)*(e->x-e->dx) + (e->y-e->dy)*(e->y-e->dy);
				
				if ((e->x>=0 && e->x<pwidth && e->y>=0 && e->y<pheight) && ((e->run==0 && dist<=lastdist) || (e->run==1 && dist>=lastdist)) && pmap[(triS32)e->y*pwidth+(triS32)e->x]==MAP_FREE)
					break;
				
				dir = (dir+1)%4;
				if (dir==lastdir)	// We tried all four possible directions and could not find a good one? (trapped!)
				{
					e->x = e->lx;
					e->y = e->ly;
					if (e->run==0 && player.berries>0 && e->gotberry==0 && lastdist<TARGET_REACH)
					{
						if (e->wait==0)
						{
							e->wait = 3;
						}
						else
						{
							e->wait--;
							if (e->wait==0)
							{
								reachedtarget();
								e->run = 1;
								e->gotberry = 1;
							}
						}
					}
					break;
				}
			}
			e->direction = dir;
			pmap[(triS32)e->ly*pwidth+(triS32)e->lx]=MAP_FREE;
			if (e->run==1 && (e->x<0 || e->x>=pwidth || e->y<0 || e->y>=pheight || pmap[(triS32)e->y*pwidth+(triS32)e->x]==MAP_UNMOVEABLE))
			{
				i++;
				continue;
			}
			pmap[(triS32)e->y*pwidth+(triS32)e->x]=MAP_ENEMIES+i;
			if (e->x!=e->lx || e->y!=e->ly)
				triImageAnimationStart( e->ani[e->direction] );
		}
		i++;
	}
}


void renderenemies( triS32 csx, triS32 csy )
{
	triS32 i = 0;
	while(i<enemies_idx)
	{
		enemy* e = &enemies[enemies_stack[i]];
		triFloat lerp = triTimerPeekDeltaTime( e->timer ) * e->speed;
		triFloat ex = (e->lx + (e->x-e->lx)*lerp)*csx;
		triFloat ey = (e->ly + (e->y-e->ly)*lerp)*csy;
		triS32 exx = (triS32)ex;
		triS32 eyy = (triS32)ey;
		triDrawImageAnimation( (exx - (32-csx)/2), (eyy - (24-csy)), e->ani[e->direction] );

		triFontActivate( verdana10 );
		triFontPrintf( verdana10, exx, (eyy + 7), 0xFFFFFFFF, "%.1f", e->hp/*, e->freeze, e->wait*/ );
		triImageAnimationUpdate( e->ani[e->direction] );
		i++;
	}
}


typedef struct
{
	triImage*			img;
	triS32				ix,iy,dx,dy;
	triS32				type;
	
	triFloat			force;
	triFloat			x, y;
	triFloat			vx, vy;
} particle;


#define MAX_PARTICLES 512
particle bubbles[MAX_PARTICLES];
triS32 bubbles_stack[MAX_PARTICLES];
particle water[MAX_PARTICLES];
triS32 water_stack[MAX_PARTICLES];

triS32 bubbles_idx = 0;
triS32 water_idx = 0;

void initparticles()
{
	triS32 i;
	for (i=0;i<MAX_PARTICLES;i++)
	{
		bubbles_stack[i] = i;
		water_stack[i] = i;
	}
}

void hitobject( triS32 n, particle* p )
{
	if (n>=MAP_ENEMIES)
	{
		hitenemy( n-MAP_ENEMIES, p->vx, p->vy, p->force, p->type );
	}
	p->force = 0.002f;
	p->vx = -p->vx*0.05f + (rand()%3-1)*p->vy*0.05f;
	p->vy = -p->vy*0.05f + (rand()%3-1)*p->vx*0.05f;
}


void addbubble( triImage* img, triFloat dx, triS32 i )
{
	if (bubbles_idx>=MAX_PARTICLES)
		return;
	
	triS32 idx = bubbles_stack[bubbles_idx++];
	
	bubbles[idx].type = ATTACK_BUBBLE;
	bubbles[idx].img = img;
	bubbles[idx].ix = (i%3) * (img->width/3);
	bubbles[idx].iy = 0;
	bubbles[idx].dx = bubbles[idx].ix + (img->width/3);
	bubbles[idx].dy = img->height;
	
	bubbles[idx].force = 0.5f +  player.level*0.1f;
	bubbles[idx].x = (triS32)player.x;
	bubbles[idx].y = (triS32)player.y;
	bubbles[idx].vx = 0.f;
	bubbles[idx].vy = 0.f;
	i += rand()%7;
	switch (player.direction)
	{
		case DIR_DOWN:
			bubbles[idx].vy = 6.5f;
			bubbles[idx].x += dx + 0.2f;
			bubbles[idx].y += 0.8f + (i&1)*0.3f + (i&2)*0.15f;
			break;
		case DIR_UP:
			bubbles[idx].vy = -6.5f;
			bubbles[idx].x += dx + 0.2f;
			bubbles[idx].y -= 0.05f + (i&1)*0.3f + (i&2)*0.15f;
			break;
		case DIR_LEFT:
			bubbles[idx].vx = -6.5f;
			bubbles[idx].y += dx + 0.2f;
			bubbles[idx].x -= 0.05f + (i&1)*0.3f + (i&2)*0.15f;
			break;
		case DIR_RIGHT:
			bubbles[idx].vx = 6.5f;
			bubbles[idx].y += dx + 0.2f;
			bubbles[idx].x += 0.8f + (i&1)*0.3f + (i&2)*0.15f;
			break;
	}
	triFloat nx = bubbles[idx].x, ny = bubbles[idx].y;
	if (nx < 0.f || nx >= pwidth || ny < 0.f || ny >= pheight)
	{
		bubbles_idx--;
		return;
	}
	triS32 obj = pmap[(triS32)ny*pwidth+(triS32)nx];
	if (obj!=MAP_FREE/* && obj!=MAP_PLAYER*/)
	{
		hitobject( obj, &bubbles[idx] );
	}
}

void addwater( triImage* img )
{
	if (water_idx>=MAX_PARTICLES)
		return;
	
	triS32 idx = water_stack[water_idx++];
	water[idx].type = ATTACK_WATERGUN;
	water[idx].img = img;
	water[idx].ix = rand()%3 * (img->width/3);
	water[idx].iy = 0;
	water[idx].dx = water[idx].ix + (img->width/3);
	water[idx].dy = img->height;
	
	water[idx].force = 10.0f + player.level;
	water[idx].x = player.x;
	water[idx].y = player.y;
	water[idx].vx = 0.f;
	water[idx].vy = 0.f;
	switch (player.direction)
	{
		case 0:
			water[idx].vy = 2.5f;
			break;
		case 1:
			water[idx].vy = -2.5f;
			break;
		case 2:
			water[idx].vx = -2.5f;
			break;
		case 3:
			water[idx].vx = 2.5f;
			break;
	}
}


void removebubble( triS32 idx )
{
	if (bubbles_idx<=0) return;
	bubbles_idx--;
	triS32 temp = bubbles_stack[bubbles_idx];
	bubbles_stack[bubbles_idx] = bubbles_stack[idx];
	bubbles_stack[idx] = temp;
}

void removewater( triS32 idx )
{
	if (water_idx<=0) return;
	water_idx--;
	triS32 temp = water_stack[water_idx];
	water_stack[water_idx] = water_stack[idx];
	water_stack[idx] = temp;
}


void updateparticles( triFloat dt )
{
	if (bubbles_idx==0 && water_idx==0) return;
	triS32 i = 0;
	
	while (i<bubbles_idx)
	{
		particle* p = &bubbles[bubbles_stack[i]];
		p->force -= dt;
		if (p->force<=0.0f) //-0.1f)
		{
			removebubble( i );
			continue;
		}

		triFloat nx = p->x + p->vx*dt;
		triFloat ny = p->y + p->vy*dt;
		if (nx < 0.f || nx >= pwidth || ny < 0.f || ny >= pheight)
		{
			removebubble( i );
			continue;
		}
		if (p->force>0.f)
		{
			triS32 obj = pmap[(triS32)ny*pwidth+(triS32)nx];
			if (obj!=0)
			{
				hitobject( obj, p );
			}
		}
		p->x = nx;
		p->y = ny;
		i++;
	}
}

void renderparticles( triS32 csx, triS32 csy )
{
	if (bubbles_idx==0 && water_idx==0) return;
	triS32 i = 0;
	
	while (i<bubbles_idx)
	{
		particle* p = &bubbles[bubbles_stack[i]];
		if (p!=0)
		triDrawImage( (triS32)(p->x*csx), (triS32)(p->y*csy), p->img->width/3, p->img->height, p->ix+0.5f, p->iy+0.5f, p->dx, p->dy, p->img );
		i++;
	}
	
	i = 0;
	while (i<water_idx)
	{
		particle* p = &water[water_stack[i]];
		if (p!=0)
		triDrawImage( (triS32)(p->x*csx), (triS32)(p->y*csy), p->img->width/3, p->img->height, p->ix, p->iy, p->dx, p->dy, p->img );
		i++;
	}
}


triS32 raiselevel()
{
	if (player.level>=99) return 0;
	
	player.exp -= player.level*10.f;
	if (player.exp<0.f) player.exp = 0.f;
	player.hp += 2;
	
	player.level++;
	return 1;
}

void learnattacks()
{
	if (player.level==5)
	{
		player.attacks |= ATTACK_BUBBLE;
		msgbox( 120, (272-50)/2, 240, 2, solid[SIZE_10], PSP_CTRL_START, "Learned a new attack!\nBUBBLE" );
	}
	if (player.level==10)
	{
		player.attacks |= ATTACK_SLAP;
		msgbox( 120, (272-50)/2, 240, 2, solid[SIZE_10], PSP_CTRL_START, "Learned a new attack!\nSLAP" );
	}
	if (player.level==17)
	{
		player.attacks |= ATTACK_WATERGUN;
		msgbox( 120, (272-50)/2, 240, 2, solid[SIZE_10], PSP_CTRL_START, "Learned a new attack!\nWATERGUN" );
	}
	if (player.level==33)
	{
		player.attacks |= ATTACK_WHIRLPOOL;
		msgbox( 120, (272-50)/2, 240, 2, solid[SIZE_10], PSP_CTRL_START, "Learned a new attack!\nWHIRLPOOL" );
	}
	if (player.level==42)
	{
		player.attacks |= ATTACK_HYDROPUMP;
		msgbox( 120, (272-50)/2, 240, 2, solid[SIZE_10], PSP_CTRL_START, "Learned a new attack!\nHYDROPUMP" );
	}
}

void levelup()
{
	if (raiselevel())
		msgbox( 120, (272-50)/2, 240, 2, solid[SIZE_10], PSP_CTRL_START, "Level up!\nYou are stronger now and can deal more damage. Also, you get more aqua points and they regenerate faster." );
	learnattacks();
}

void qlevelup()
{
	raiselevel();
	learnattacks();
}


void main_loop()
{
	// Copy frontbuffer -> backbuffer
	triCopyFromScreen( 0, 0, 480, 272, 0, 0, 512, triFramebuffer );
	drawmsgbox( (480-240)/2, (272-34)/2, 240, 1, solid[SIZE_10], "Loading..." );
	triSwapbuffers();
	
	initparticles();
	initenemies();
	
	triS32 i;
	triImage* bubbles = triImageLoad( "data/bubbles.png", TRI_VRAM|TRI_SWIZZLE );
	if (bubbles==0)
		return;
	
	triImage* status = triImageLoad( "data/statusbar.png", TRI_VRAM|TRI_SWIZZLE );
	if (status==0)
		return;
	
	player.spritesheet = triImageLoad( "data/mudkip_spritesheet.png", TRI_VRAM|TRI_SWIZZLE );
	if (player.spritesheet==0)
		return;
	for (i=0;i<4;i++)
	{
		player.ani[i] = triImageAnimationFromSheet2( player.spritesheet, 0, i*27, 20, 27, 4, 1, 150/4 );
		player.ani[i]->loops = 1;
	}
	player.level = 1;
	player.exp = 0;
	player.hp = 5;
	player.ap = 100;
	player.attacks = ATTACK_TACKLE;
	player.berries = 5;
	player.direction = 0;
	player.speed = 0;
	player.timer = triTimerCreate();
	
	triImage* background = triImageLoad( "data/grassland.png", TRI_VRAM|TRI_SWIZZLE );
	triImage* foreground = triImageLoad( "data/grassland_layer2.png", TRI_VRAM|TRI_SWIZZLE );
	triImage* collision = triImageLoad( "data/grassland_collision.png", 0 );
	if (collision==0 || background==0 || foreground==0)
	{
		triLogPrint("Could not load map!\n");
		return;
	}
	if (collision->format!=IMG_FORMAT_T8)
	{
		triLogPrint("Collision map not in T8 format!\n");
		return;
	}
	triChar* cmap = (triChar*)collision->data;
	
	triS32 csx = 480 / collision->width;
	triS32 csy = 272 / collision->height;
	triFloat speedfactor = 16.f / csx;
	
	triLogPrint("csx: %i\ncsy: %i\n", csx, csy );
	player.x = player.lx = 240.f * collision->width / 480.f;
	player.y = player.ly = 192.f * collision->height / 272.f;
	
	pwidth = collision->width;
	pheight = collision->height;
	pmap = triMalloc( pwidth*pheight );
	if (pmap==0)
	{
		triLogPrint("Error allocating playfield map!\n");
		return;
	}
	memset( pmap, 0, pwidth*pheight );
	
	triS32 xx, yy;
	for (yy=0;yy<pheight;yy++)
	for (xx=0;xx<pwidth;xx++)
		pmap[yy*pwidth+xx] = !cmap[yy*collision->stride+xx];

	triImagePaletteSet( collision, 0, 0xFF, 0xFF, 0xFF, 0xFF );
	triImagePaletteSet( collision, 1, 0x00, 0x00, 0x00, 0xFF );
	triImagePaletteSet( collision, 2, 0x00, 0x00, 0xFF, 0xFF );
	for (yy=MAP_ENEMIES;yy<256;yy++)
		triImagePaletteSet( collision, yy, 0xFF, 0x00, 0x00, 0xFF );

	while (pmap[(triS32)player.y*pwidth + (triS32)player.x]!=0)
	{
		player.y += 1.0f;
	}
	triLogPrint("Placed player.\n");

	triS32 firstframe = 1;
	triFloat ymin = -(background->height-272);
	if (ymin>0) ymin = 0.f;
	triFloat y = 0.f;
	while(y>=ymin)
	{
		triDrawImage2( 0, y, background );
		triDrawImageAnimation( player.x*csx - (20-csx)/2, player.y*csy - (27-csy) + (background->height-272) + y, player.ani[player.direction] );
	
		if (firstframe)
		{
			firstframe = 0;
			//triSync();
			fadefromcol( 0xFFFFFF, 1.0f );
			continue;
		}
		
		y -= 0.5f;
		triSwapbuffers();
	}
	y = ymin;
	
	msgbox( (480-240)/2, (272-64)/2, 240, 3, solid[SIZE_10], PSP_CTRL_CROSS, "Stage 1: Save your food!\nWild Pokemon are trying to steal your collected food, so you have to keep them away. If they get near your berries, they will steal them and try to run away, but you can still get the berry back before they get out of reach!\nMove with the D-Pad and press the round buttons to use your attacks." );
	
	triTimer* frametimer = triTimerCreate();
	if (frametimer==0)
	{
		triLogPrint("Error creating frame timer!\n");
		return;
	}
	
	while(isrunning && !isgameover)
	{
		triDrawImage2( 0, y, background );
		triFloat lerp = triTimerPeekDeltaTime( player.timer );
		if (lerp==0) lerp = 0.15f;
		triDrawImageAnimation( (triS32)((player.lx + (player.x-player.lx)*lerp/0.15f)*csx - (20-csx)/2), (triS32)((player.ly + (player.y-player.ly)*lerp/0.15f)*csy - (27-csy)), player.ani[player.direction] );
		triDrawImage2( 0, y, foreground );
		
		#ifdef DEBUG_CONSOLE
		if (triCVarGetf( "debugcollisionmap" )!=0.0f)
		{
			triS32 xx, yy;
			for (yy=0;yy<pheight;yy++)
			for (xx=0;xx<pwidth;xx++)
				cmap[yy*collision->stride+xx] = pmap[yy*pwidth+xx];
			sceKernelDcacheWritebackAll();
			triImageConstAlpha( 127 );
			sceGuTexFilter(GU_NEAREST,GU_NEAREST);
			triDrawImage( 0, 0, 480, 272, 0, 0, collision->width, collision->height, collision );
			sceGuTexFilter(GU_LINEAR,GU_LINEAR);
			triImageNoTint();
		}
		#endif
		renderenemies( csx, csy );
		renderparticles( csx, csy );
		
		triDrawImage( 480-112, 0, 111, 39,  1, 1, 112, 40, status );
		triFloat hp = player.hp / (3.f + player.level*2.f);
		if (hp>1.0f) hp = 1.0f;
		triFloat ap = player.ap / (48+2*player.level);
		if (ap>1.0f) ap = 1.0f;
		triFloat exp = player.exp / (player.level*10.f - 1.0f);
		triDrawImage( 480-112+51, 19, hp*48, 3,  52 + hp*47, 44, 53 + hp*47, 47, status );	// HP
		triDrawImage( 480-112+51, 27, ap*48, 3,  52 + ap*47, 48, 53 + ap*47, 51, status );	// AP
		triDrawImage( 480-112+35, 35, exp*64, 2,  36, 41, 36 + exp*64, 43, status );	// EXP
		triFontActivate( verdana10 );
		triFontPrint( verdana10, 480-112+18, 6, STATUSSHADOW, "Mudkip" );
		triFontPrint( verdana10, 480-112+17, 5, STATUSCOLOR, "Mudkip" );
		triFontPrintf( verdana10, 480-112+84, 6, STATUSSHADOW, "%i", player.level );
		triFontPrintf( verdana10, 480-112+83, 5, STATUSCOLOR, "%i", player.level );
		triFontPrintf( verdana10, 480-112+19, 21, STATUSSHADOW, "%i", player.berries );
		triFontPrintf( verdana10, 480-112+18, 20, STATUSCOLOR, "%i", player.berries );
		
		triFontActivate( solid[SIZE_10] );
		triFontPrintf( solid[SIZE_10], 1, 1, 0xFF000000, "enemies: %i\n", enemies_active );
		triFontPrintf( solid[SIZE_10], 0, 0, 0xFFFFFFFF, "enemies: %i\n", enemies_active );
		
		triImageAnimationUpdate( player.ani[player.direction] );
		
		triInputUpdate();
		if (lerp>=0.15f)
		{
			player.ly = player.y;
			player.lx = player.x;
			triTimerUpdate( player.timer );
			if (!triInputHeld(PSP_CTRL_CROSS))
			{
				if (triInputHeld(PSP_CTRL_LEFT))
				{
					player.direction = DIR_LEFT;
					player.speed = speedfactor;
					player.x -= speedfactor;
					if (player.x<0) player.x=0;
					if (pmap[(triS32)player.y*pwidth + (triS32)player.x]!=0)
					{
						player.x = player.lx;
					}
					else
					{
						pmap[(triS32)player.ly*pwidth+(triS32)player.lx]=MAP_FREE;
						pmap[(triS32)player.y*pwidth+(triS32)player.x]=MAP_PLAYER;
					}
					if(triImageAnimationIsDone( player.ani[player.direction]))
						triImageAnimationReset( player.ani[player.direction] );
					triImageAnimationStart( player.ani[player.direction] );
				}
				else
				if (triInputHeld(PSP_CTRL_RIGHT))
				{
					player.direction = DIR_RIGHT;
					player.speed = speedfactor;
					player.x += speedfactor;
					if (player.x>=pwidth) player.x=pwidth-1;
					if (pmap[(triS32)player.y*pwidth + (triS32)player.x]!=0)
					{
						player.x = player.lx;
					}
					else
					{
						pmap[(triS32)player.ly*pwidth+(triS32)player.lx]=MAP_FREE;
						pmap[(triS32)player.y*pwidth+(triS32)player.x]=MAP_PLAYER;
					}
					if(triImageAnimationIsDone( player.ani[player.direction]))
						triImageAnimationReset( player.ani[player.direction] );
					triImageAnimationStart( player.ani[player.direction] );
				}
				else
				if (triInputHeld(PSP_CTRL_UP))
				{
					player.direction = DIR_UP;
					player.speed = speedfactor;
					player.y -= speedfactor;
					if (player.x<0) player.x=0;
					if (pmap[(triS32)player.y*pwidth + (triS32)player.x]!=0)
					{
						player.y = player.ly;
					}
					else
					{
						pmap[(triS32)player.ly*pwidth+(triS32)player.lx]=MAP_FREE;
						pmap[(triS32)player.y*pwidth+(triS32)player.x]=MAP_PLAYER;
					}
					if(triImageAnimationIsDone( player.ani[player.direction]))
						triImageAnimationReset( player.ani[player.direction] );
					triImageAnimationStart( player.ani[player.direction] );
				}
				else
				if (triInputHeld(PSP_CTRL_DOWN))
				{
					player.direction = DIR_DOWN;
					player.speed = speedfactor;
					player.y += speedfactor;
					if (player.y>=pheight) player.y=pheight-1;
					if (pmap[(triS32)player.y*pwidth + (triS32)player.x]!=0)
					{
						player.y = player.ly;
					}
					else
					{
						pmap[(triS32)player.ly*pwidth+(triS32)player.lx]=MAP_FREE;
						pmap[(triS32)player.y*pwidth+(triS32)player.x]=MAP_PLAYER;
					}
					if(triImageAnimationIsDone( player.ani[player.direction]))
						triImageAnimationReset( player.ani[player.direction] );
					triImageAnimationStart( player.ani[player.direction] );
				}
			}
			else
			{
				triS32 t = 0;
				for (t=0;t<5;t++)
				{
					if (player.ap>0)
					{
						addbubble( bubbles, (t-2)/16.f, t );
						#ifdef DEBUG_CONSOLE
						if (!triCVarGetf("unlimitedaqua")==1.0f)
						#endif
						player.ap -= 1;
					}
				}
			}
			
			player.ap+=(0.9f+player.level*0.2f);	// at level 21 ap is renegerated as fast as bubble consumes
			if (player.ap>(48+2*player.level)) 
				player.ap=(48+2*player.level);
			
			if ((triS32)((rand()%MAX_ENEMIES)*40*lerp)>0)
				spawnenemy( rand()%MAX_TYPES, player.level-2>0?player.level-2:1, player.level+5, 15, 9 );
		}
		if(triImageAnimationIsDone( player.ani[player.direction]))
			triImageAnimationReset( player.ani[player.direction] );
		

		#ifdef DEBUG_CONSOLE
		triConsoleUpdate();		
		if (triInputHeld(PSP_CTRL_SELECT) && triInputHeld(PSP_CTRL_LTRIGGER) && triInputHeld(PSP_CTRL_RTRIGGER))
			triConsoleToggle();
		else
		#endif
		if (triInputPressed(PSP_CTRL_SELECT))
		{
			msgbox( 120, (272-27)/2, 240, 1, solid[SIZE_10], PSP_CTRL_SELECT, "               GAME PAUSED" );
		}
		if (triInputPressed(PSP_CTRL_START))
		{
			gameover();
		}
		
		triTimerUpdate( frametimer );
		updateparticles( triTimerGetDeltaTime( frametimer ) );
		updateenemies( triTimerGetDeltaTime( frametimer ) );
		
		if (berriesinplay()==0)
			gameover();
		
		if (player.exp>=player.level*10.0f)
			levelup();
		
		triSwapbuffers();
	}
	
	triFree( pmap );
	isgameover = 0;
	pmap = 0;
	triImageFree( collision );
	triImageFree( bubbles );
	triImageFree( status );
	triImageFree( background );
	triImageFree( foreground );
	triImageFree( player.spritesheet );
	for (i=0;i<4;i++)
		triImageAnimationFree( player.ani[i] );
	deinitenemies();
	triTimerFree( player.timer );
	triTimerFree( frametimer );
	if (isrunning)
		fadetocol( 0xFFFFFF, 1.0f );
}


void startgame()
{
	main_loop();
}


#define NUM_CREDITS_LINES 51
triChar* Credits[NUM_CREDITS_LINES] =
		{
"                                         ",
"              A game made in 72 hours.",
"",
"",
"Game code:   Raphael",
"",
"Game engine: Tomaz",
"              Raphael",
"              InsertWittyName",
"",
"Graphics: www.spriters-resource.com",
"",
"Sounds: some free wav archive",
"",
"",
"",
"Thanks: Guys from psp-programming.com",
"         for the idea",
"         esp \"Mudkip-natic\" Ryalla :P",
"         and Zettablade for finding the",
"         sprites",
"",
"         All of ps2dev.org for making",
"         this possible at all.",
"",
"         TyRaNiD for PSPLink - couldn’t",
"         imagine coding on PSP without",
"",
"         My loving girlfriend who’s having",
"         some hard time with my coding",
"         madness ;) I love you",
"",
"",
"Greets: The rest of psp-programming.com",
"         ps2dev.org folks",
"         Anyone that dedicated some of his",
"         time for the PSP-Scene",
"",         
"",
"",
"         ...not the QJ-noobs :P",
"", 
"",
"First release: gbax‘07 coding competition",
"",
"",
"This is free software. Any selling or renting",
"is strictly prohibited.",
"If you paid for this, bad luck for you!",
"",
"Thanks for playing."
		};

void credits()
{
	triS32 fontsize = SIZE_14;
	triFont* font = solid[fontsize];
	if (font==0) return;
	
	triImage* background = triImageLoad( "data/menuback.png", /*TRI_VRAM|*/TRI_SWIZZLE );
	triImage* title = triImageLoad( "data/title.png", /*TRI_VRAM|*/TRI_SWIZZLE );
	if (background==0 || title==0) return;
	
	triFloat y = 280.0f;
	triDrawImage2( 0, 0, background );
	triDrawImage( (480-title->width)*0.5f, y, title->width, title->height,
					0.5f, 0.5f, title->width-0.5f, title->height-0.5f, title );
	
	triFloat x = 240 - triFontMeasureText( font, Credits[0] )*0.5f;
	triFloat ty = y + title->height + 10.f;
	triS32 line = 0;

	triFontActivate( font );
	while (ty<272.f && line<NUM_CREDITS_LINES)
	{
		if (Credits[line] && Credits[line][0]!='\0' && Credits[line][0]!='\n')
		{
			triFontPrint( font, x+2, ty+2, 0xFF000000, Credits[line] );
			triFontPrint( font, x, ty, 0xFFFFFFFF, Credits[line] );
		}
		line++;
		ty += font->fontHeight;
	}
	triSync();
	fadefromcol( 0xFFFFFF, 1.0f );
	
	
	triFloat lasty = 0;
	while (lasty>-60.f && isrunning)
	{
		y -= 0.5f;
		sceGuTexFunc( GU_TFX_REPLACE, GU_TCC_RGBA );
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);
		
		triDrawImage2( 0, 0, background );
		if (y > -title->height)
			triDrawImage( (480-title->width)*0.5f, y, title->width, title->height,
						0.5f, 0.5f, title->width-0.5f, title->height-0.5f, title );
		
		triFloat ty = y + title->height + 10.f;
		triS32 line = 0;
		while (ty<-font->fontHeight && line<NUM_CREDITS_LINES)
		{
			line++;
			ty += font->fontHeight;
		}
		
		triFontActivate( font );
		while (ty<272.f && line<NUM_CREDITS_LINES)
		{
			if (Credits[line] && Credits[line][0]!='\0' && Credits[line][0]!='\n')
			{
				triFontPrint( font, x+2, ty+2, 0xFF000000, Credits[line] );
				triFontPrint( font, x, ty, 0xFFFFFFFF, Credits[line] );
			}
			line++;
			ty += font->fontHeight;
		}
		lasty = ty;
		triSwapbuffers();
		triInputUpdate();
		if (triInputPressed(PSP_CTRL_CROSS))
			break;
	}
	sceGuTexFunc( GU_TFX_REPLACE, GU_TCC_RGBA );
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	
	triImageFree( background );
	triImageFree( title );
	if (isrunning)
		fadetocol( 0xFFFFFF, 1.0f );
}


void splash( triChar* filename )
{
	triImage* splash = triImageLoad( filename, TRI_SWIZZLE|TRI_VRAM );
	triDrawImage2( 0, 0, splash );
	triSync();
	triImageFree( splash );
	fadefromcol( 0, 1.0f );
	sceKernelDelayThread( 25*100*1000 );
	fadetocol( 0, 1.0f );
}


void intro()
{
	#ifndef NOSPLASHES
	splash( "data/PSP-2_rokdcasbah.png" );
	#endif
	
	msgborder = triImageLoad( "data/msgborder.png", TRI_VRAM|TRI_SWIZZLE );
	if (msgborder==0)
	{
		triLogPrint("Error loading message border image!\n");
		return;
	}
	
	if (!triAt3Load( "data/ustv.at3" ))
	{
		triLogPrint("Error loading background music!\n");
		return;
	}
	triAt3SetLoop( 1 );
	
	triImage* background = triImageLoad( "data/menuback.png", /*TRI_VRAM|*/TRI_SWIZZLE );
	triImage* mudkip1 = triImageLoad( "data/mudkip.png", TRI_SWIZZLE );
	triImage* mudkip2 = triImageLoad( "data/mudkip2.png", TRI_SWIZZLE );
	triImage* title = triImageLoad( "data/title.png", TRI_SWIZZLE );
	if (background==0 || mudkip1==0 || mudkip2==0 || title==0)
	{
		triLogPrint("Error loading menuscreen images!\n");
		return;
	}
	
	triImage* menum = triImageLoad( "data/menu.png", /*TRI_VRAM|*/TRI_SWIZZLE );
	if (menum==0)
	{
		triLogPrint("Error loading menu item images!\n");
		return;
	}
	
	triWav* select = triWavLoad( "data/start.wav" );
	triWav* click = triWavLoad( "data/click.wav" );
	
	triS32 first = 1;
	while (isrunning)
	{
		triS32 menuitem = 0;
		triFloat titlescale = 1.0f;
		triFloat menuscale[3] = { 1.0f, 1.0f, 1.0f };
		triEnable(TRI_VBLANK);
		triTimer* timer = triTimerCreate();
		triS32 i = 0;
		
		triAt3Play();
		while (isrunning)
		{
			triDrawImage2( 0, 0, background );
			if (((triU32)(triTimerPeekDeltaTime( timer )*4.f)) % 12 == 1)
				triDrawImage2( 280, 66, mudkip2 );
			else
				triDrawImage2( 280, 66, mudkip1 );
			triDrawImage( (480-title->width*titlescale)*0.5f, (115-title->height*titlescale)*0.5f, title->width*titlescale, title->height*titlescale,
							0.5f, 0.5f, title->width-0.5f, title->height-0.5f, title );
			for(i=0;i<3;i++)
			{
				if (i==menuitem)
				{
					menuscale[i] += (1.05f - menuscale[i])*0.1f;
					triDrawImage( 32 - (menuscale[i]-1.0f)*6.0f, i*40 + 127 - (menuscale[i]-1.0f)*1.0f, menum->width*menuscale[i], 40*menuscale[i],
							0.5f, i*80+40+0.5f, menum->width-0.5f, i*80+40+40-0.5f, menum );
				}
				else
				{
					menuscale[i] -= (menuscale[i] - 1.0f)*0.1f;
					triDrawImage( 32 - (menuscale[i]-1.0f)*6.0f, i*40 + 127 - (menuscale[i]-1.0f)*1.0f, menum->width*menuscale[i], 40*menuscale[i],
							0.5f, i*80+0.5f, menum->width-0.5f, i*80+40-0.5f, menum );
				}
			}
			
			
			if (first==1)
			{
				triSync();
				// Fade in from black (on game start)
				fadefromcol( 0, 1.5f );
				triTimerUpdate( timer );
				first = 0;
			}
			else
			if (first==2)
			{
				triSync();
				// Fade in from white (on menu change)
				fadefromcol( 0xFFFFFF, 1.0f );
				triTimerUpdate( timer );
				first = 0;
			}
			
			
			triInputUpdate();
			if (triInputPressed(PSP_CTRL_UP))
			{
				triWavPlay( select );
				menuitem--;
				if (menuitem<0) menuitem = 2;
			}
			else
			if (triInputPressed(PSP_CTRL_DOWN))
			{
				triWavPlay( select );
				menuitem = (menuitem+1) % 3;
			}
			else
			if (triInputPressed(PSP_CTRL_CROSS))
			{
				triWavPlay( click );
				sceKernelDelayThread(500000);
				break;
			}

			triSwapbuffers();
			titlescale = 1.0f + 0.05f*sinf( triTimerPeekDeltaTime(timer) * M_PI * 0.5f );
		}
		
		triTimerFree( timer );

		if (!isrunning || menuitem==2)
			break;

		if (menuitem==1)
		{
			fadetocol( 0xFFFFFF, 1.0f );
			credits();
			first = 2;
		}
		
		if (menuitem==0)
		{
			// Unload unneeded data for the main game
			//triImageFree( background );
			//triImageFree( mudkip1 );
			//triImageFree( mudkip2 );
			//triImageFree( title );
			fadetocol( 0xFFFFFF, 1.0f );
			main_loop();
			//background = triImageLoad( "data/menuback.png", TRI_VRAM|TRI_SWIZZLE );
			//mudkip1 = triImageLoad( "data/mudkip.png", TRI_VRAM|TRI_SWIZZLE );
			//mudkip2 = triImageLoad( "data/mudkip2.png", TRI_VRAM|TRI_SWIZZLE );
			//title = triImageLoad( "data/title.png", TRI_VRAM|TRI_SWIZZLE );
			first = 2;
		}
	}
	
	triWavStopAll();
	triWavFree( select );
	triWavFree( click );
	triImageFree( menum );
	
	triImageFree( background );
	triImageFree( mudkip1 );
	triImageFree( mudkip2 );
	triImageFree( title );
	triImageFree( msgborder );
	triAt3Free();
}


int main(int argc, char *argv[])
{
	init();
	intro();
	deinit();
	sceKernelExitGame();
	return(0);
}
