/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

/** \file blender/blenloader/intern/versioning_upbge.c
 *  \ingroup blenloader
 */

#include "BLI_utildefines.h"
#include "BLI_compiler_attrs.h"

#include <stdio.h>

/* allow readfile to use deprecated functionality */
#define DNA_DEPRECATED_ALLOW

#include "DNA_genfile.h"
#include "DNA_material_types.h"
#include "DNA_object_types.h"
#include "DNA_sdna_types.h"
#include "DNA_sensor_types.h"
#include "DNA_space_types.h"
#include "DNA_mesh_types.h"
#include "DNA_material_types.h"

#include "BKE_main.h"

#include "BLO_readfile.h"

#include "wm_event_types.h"

#include "readfile.h"

#include "MEM_guardedalloc.h"

void blo_do_versions_upbge(FileData *fd, Library *lib, Main *main)
{
	//printf("UPBGE: open file from version : %i, subversion : %i\n", main->upbgeversionfile, main->upbgesubversionfile);
	if (!MAIN_VERSION_UPBGE_ATLEAST(main, 0, 1)) {
		if (!DNA_struct_elem_find(fd->filesdna, "bRaySensor", "int", "mask")) {
			bRaySensor *raySensor;

			for (Object *ob = main->object.first; ob; ob = ob->id.next) {
				for (bSensor* sensor = ob->sensors.first; sensor != NULL; sensor = (bSensor *)sensor->next) {
					if (sensor->type == SENS_RAY) {
						raySensor = (bRaySensor *)sensor->data;
						raySensor->mask = 0xFFFF;//all one, 'cause this was the previous behavior
					}
				}
			}
		}
		for (Scene *scene = main->scene.first; scene; scene = scene->id.next) {
			/* Previous value of GAME_GLSL_NO_ENV_LIGHTING was 1 << 18, it was conflicting
			 * with GAME_SHOW_BOUNDING_BOX. To fix this issue, we replace 1 << 18 by
			 * 1 << 21 (the new value) when the file come from blender not UPBGE.
			 */
			if (scene->gm.flag & (1 << 18)) {
				scene->gm.flag |= GAME_GLSL_NO_ENV_LIGHTING;
				/* Disable bit 18 */
				scene->gm.flag &= ~(1 << 18);
			}
		}
	}

	if (!MAIN_VERSION_UPBGE_ATLEAST(main, 0, 6)) {
		if (!DNA_struct_elem_find(fd->filesdna, "Material", "short", "constflag")) {
			for (Material *ma = main->mat.first; ma; ma = ma->id.next) {
				ma->constflag |= MA_CONSTANT_TEXTURE;
			}
		}
	}

	if (!MAIN_VERSION_UPBGE_ATLEAST(main, 0, 9)) {
		if (!DNA_struct_elem_find(fd->filesdna, "GameData", "short", "pythonkeys[4]")) {
			for (Scene *scene = main->scene.first; scene; scene = scene->id.next) {
				scene->gm.pythonkeys[0] = LEFTCTRLKEY;
				scene->gm.pythonkeys[1] = LEFTSHIFTKEY;
				scene->gm.pythonkeys[2] = LEFTALTKEY;
				scene->gm.pythonkeys[3] = TKEY;
			}
		}
	}
	if (!MAIN_VERSION_UPBGE_ATLEAST(main, 0, 10)) {
		if (!DNA_struct_elem_find(fd->filesdna, "Material", "float", "depthtranspfactor")) {
			for (Material *ma = main->mat.first; ma; ma = ma->id.next) {
				ma->depthtranspfactor = 1.0f;
			}
		}

		if (!DNA_struct_elem_find(fd->filesdna, "EnvMap", "short", "flag")) {
			for (Tex *tex = main->tex.first; tex; tex = tex->id.next) {
				if (tex->env) {
					tex->env->flag |= ENVMAP_AUTO_UPDATE;
				}
			}
		}
	}

	if (!MAIN_VERSION_UPBGE_ATLEAST(main, 0, 11)) {
		if (!DNA_struct_elem_find(fd->filesdna, "MTex", "float", "ior")) {
			for (Material *ma = main->mat.first; ma; ma = ma->id.next) {
				for (unsigned short a = 0; a < MAX_MTEX; ++a) {
					if (ma->mtex[a]) {
						ma->mtex[a]->ior = 1.0f;
					}
				}
			}
		}
	}

	if (!MAIN_VERSION_UPBGE_ATLEAST(main, 0, 12)) {
		if (!DNA_struct_elem_find(fd->filesdna, "Object", "float", "friction")) {
			for (Object *ob = main->object.first; ob; ob = ob->id.next) {
				if (ob->type == OB_MESH) {
					Mesh *me = blo_do_versions_newlibadr(fd, lib, ob->data);
					for (unsigned short i = 0; i < me->totcol; ++i) {
						Material *ma = blo_do_versions_newlibadr(fd, lib, me->mat[i]);
						if (ma) {
							ob->friction = ma->friction;
							ob->rolling_friction = ma->rolling_friction;
							ob->fh = ma->fh;
							ob->reflect = ma->reflect;
							ob->fhdist = ma->fhdist;
							ob->xyfrict = ma->xyfrict;
							if (ma->dynamode & MA_FH_NOR) {
								ob->dynamode |= OB_FH_NOR;
							}
							break;
						}
					}
				}
			}
		}
	}
}
