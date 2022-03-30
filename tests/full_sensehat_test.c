#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>

static uint8_t screen[8][8][3];

static int disp_fd, joy_fd;

static void draw_loop(void);
static void update_screen(void);

int main(int argc, char **argv)
{
	char *disp_filename = "/dev/sense-hat";
	char *joy_filename = "/dev/input/event0";
	switch(argc)
	{
	case 3:
		disp_filename = argv[2];
		[[fallthrough]];
	case 2:
		joy_filename = argv[1];
		[[fallthrough]];
	case 1:
		break;
	default:
		errx(1, "invalid arguments");
	}
	if(0 > (disp_fd = open(disp_filename, O_RDWR)))
		err(1, "unable to open %s for writing", disp_filename);
	if(0 > (joy_fd = open(joy_filename, O_RDONLY)))
		err(1, "unable to open %s for writing", joy_filename);

	draw_loop();

	memset(screen, 0, sizeof screen);
	update_screen();

	close(disp_fd);
	close(joy_fd);
	return 0;
}

void update_screen(void)
{
	if(0 > write(disp_fd, screen, sizeof screen))
		err(1, "unable to write to fb");
	if(0 > lseek(disp_fd, 0, SEEK_SET))
		err(1, "unable to seek fb");
}

typedef struct
{
	uint8_t x:3,y:3;
}
Point;

static void clear_pixel(Point loc)
{
	screen[loc.y][loc.x][0]=0;
	screen[loc.y][loc.x][1]=0;
	screen[loc.y][loc.x][2]=0;
}

typedef union
{
	uint16_t combined;
	struct
	{
		uint16_t r:5,g:5,b:5;
	};
}
Color;

static Color get_random_color(void)
{
	return (Color){.combined = (uint16_t)rand()};
}

static void set_pixel(Point loc, Color col)
{
	screen[loc.y][loc.x][0]=col.r;
	screen[loc.y][loc.x][1]=col.g;
	screen[loc.y][loc.x][2]=col.b;
}

typedef enum
{
	UP,DOWN,LEFT,RIGHT,SELECT,
}
Direction;

static Direction get_direction(void)
{
	struct input_event evt;
	do
	{
		if(0 > read(joy_fd, &evt, sizeof evt))
			err(1, "unable to read from joystick");
	}
	while(evt.type != EV_KEY || evt.value == 0); //released
	switch(evt.code)
	{
	case BTN_DPAD_UP:
		return UP;
	case BTN_DPAD_DOWN:
		return DOWN;
	case BTN_DPAD_LEFT:
		return LEFT;
	case BTN_DPAD_RIGHT:
		return RIGHT;
	case BTN_SELECT:
		return SELECT;
	default:
		err(1, "invalid button from joystick");
	}
}

void draw_loop(void)
{
	Point loc = {0,0};
	for(;;)
	{
		set_pixel(loc, get_random_color());
		update_screen();
		clear_pixel(loc);

		Point newloc = loc;
		switch(get_direction())
		{
		case UP:
			--newloc.y;
			break;
		case DOWN:
			++newloc.y;
			break;
		case LEFT:
			--newloc.x;
			break;
		case RIGHT:
			++newloc.x;
			break;
		case SELECT:
			return;
		}
		loc = newloc;
	}
}
