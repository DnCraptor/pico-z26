/*
** sdlmouse.c -- SDL mouse code
*/

db srv_mouse_button = 0;
int srv_micky_x = 0, srv_micky_y = 0;

void srv_get_mouse_movement()
{
	srv_mouse_button = 0; //SDL_GetRelativeMouseState(&srv_micky_x, &srv_micky_y);
}

/**
** z26 is Copyright 1997-2019 by John Saeger and contributors.  
** z26 is released subject to the terms and conditions of the 
** GNU General Public License Version 2 (GPL).	z26 comes with no warranty.
** Please see COPYING.TXT for details.
*/
