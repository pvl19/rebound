/**
 * @file 	rotations.c
 * @brief 	Tools for vector manipulations, rotations and quaternions.
 * @author 	Hanno Rein <hanno@hanno-rein.de>, Dan Tamayo
 * @details This code uses the same conventions as the Apple SIMD quaternion framework. See e.g.: 
 *          https://github.com/xybp888/iOS-SDKs/blob/master/iPhoneOS13.0.sdk/usr/include/simd/quaternion.h
 * 
 * @section 	LICENSE
 * Copyright (c) 2022 Hanno Rein
 *
 * This file is part of rebound.
 *
 * rebound is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rebound is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rebound.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdlib.h>
#include <math.h>
#include "rebound.h"
#include "tools.h"

// reb_vec3d manipulation functions

struct reb_vec3d reb_vec3d_mul(const struct reb_vec3d v, const double s){
    struct reb_vec3d nv = {
        .x = s*v.x,
        .y = s*v.y,
        .z = s*v.z
    };
    return nv;
}

struct reb_vec3d reb_vec3d_add(const struct reb_vec3d v, const struct reb_vec3d w){
    struct reb_vec3d nv = {
        .x = v.x + w.x,
        .y = v.y + w.y,
        .z = v.z + w.z
    };
    return nv;
}

struct reb_vec3d reb_vec3d_cross(const struct reb_vec3d a, const struct reb_vec3d b){
    struct reb_vec3d c = {
        .x = a.y*b.z - a.z*b.y,
        .y = a.z*b.x - a.x*b.z,
        .z = a.x*b.y - a.y*b.x,
    };
    return c;
}

double reb_vec3d_dot(const struct reb_vec3d a, const struct reb_vec3d b){
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

double reb_vec3d_length_squared(const struct reb_vec3d v){
    return reb_vec3d_dot(v, v);
}

struct reb_vec3d reb_vec3d_normalize(const struct reb_vec3d v){
    return reb_vec3d_mul(v, 1./sqrt(reb_vec3d_length_squared(v)));
}

// reb_rotation manipulation functions 

struct reb_vec3d reb_rotation_imag(const struct reb_rotation q){
    struct reb_vec3d i = {
        .x = q.ix,
        .y = q.iy,
        .z = q.iz
    };
    return i;
}

struct reb_rotation reb_rotation_mul(const struct reb_rotation p, const struct reb_rotation q){
    // v_rot = p * ( q * v)
    struct reb_rotation r = {
        .r  = p.r*q.r  - p.ix*q.ix - p.iy*q.iy - p.iz*q.iz,
        .ix = p.r*q.ix + p.ix*q.r  + p.iy*q.iz - p.iz*q.iy,
        .iy = p.r*q.iy - p.ix*q.iz + p.iy*q.r  + p.iz*q.ix,
        .iz = p.r*q.iz + p.ix*q.iy - p.iy*q.ix + p.iz*q.r
    };
    return r;
}

double reb_rotation_length_squared(const struct reb_rotation q){
    return q.r*q.r + q.ix*q.ix + q.iy*q.iy + q.iz*q.iz;
}

struct reb_rotation reb_rotation_conjugate(const struct reb_rotation q){
    struct reb_rotation c = {.ix = -q.ix, .iy = -q.iy, .iz = -q.iz, .r = q.r };
    return c;
}

struct reb_rotation reb_rotation_inverse(const struct reb_rotation q){
    struct reb_rotation c = reb_rotation_conjugate(q);
    double rl2 = 1./reb_rotation_length_squared(q);
    c.r *= rl2;
    c.ix *= rl2;
    c.iy *= rl2;
    c.iz *= rl2;
    return c;
}

// Object rotation functions

struct reb_vec3d reb_vec3d_rotate(struct reb_vec3d v, const struct reb_rotation q){
    // Returns a copy
    struct reb_vec3d r = v;
    reb_vec3d_irotate(&r, q);
    return r;
}

void reb_vec3d_irotate(struct reb_vec3d* v, const struct reb_rotation q){
    // Rotates vector in place
    struct reb_vec3d imag = reb_rotation_imag(q);
    struct reb_vec3d t = reb_vec3d_mul(reb_vec3d_cross(imag,*v), 2);
    struct reb_vec3d res = reb_vec3d_add(*v, reb_vec3d_add(reb_vec3d_mul(t, q.r), reb_vec3d_cross(imag, t)));
    v->x = res.x;
    v->y = res.y;
    v->z = res.z;
}

void reb_particle_irotate(struct reb_particle* p, const struct reb_rotation q){
    struct reb_vec3d pos = {p->x, p->y, p->z};
    reb_vec3d_irotate(&pos, q);
    p->x = pos.x;
    p->y = pos.y;
    p->z = pos.z;
    struct reb_vec3d vel = {p->vx, p->vy, p->vz};
    reb_vec3d_irotate(&vel, q);
    p->vx = vel.x;
    p->vy = vel.y;
    p->vz = vel.z;
}

void reb_simulation_irotate(struct reb_simulation* const sim, const struct reb_rotation q){
    const int N = sim->N;
    for (int i = 0; i < N; i++){
        struct reb_particle* p = &sim->particles[i];
        reb_particle_irotate(p,q);
    }
}

// Alternate ways of initializing rotation

struct reb_rotation reb_rotation_identity(){
    struct reb_rotation q = {.ix = 0.0, .iy = 0.0, .iz = 0.0, .r = 1.0 };
    return q;
}

static inline struct reb_rotation reb_rotation_init_from_to_reduced(const struct reb_vec3d from, const struct reb_vec3d to) {
    // Internal use only
    struct reb_vec3d half = {.x=from.x+to.x, .y=from.y+to.y, .z=from.z+to.z};
    half = reb_vec3d_normalize(half);
    struct reb_vec3d cross = reb_vec3d_cross(from, half);
    double dot = reb_vec3d_dot(from, half);
    struct reb_rotation q = {.ix=cross.x, .iy=cross.y, .iz=cross.z, .r=dot};
    return q;
}

struct reb_rotation reb_rotation_init_from_to(struct reb_vec3d from, struct reb_vec3d to) {
    from = reb_vec3d_normalize(from);
    to = reb_vec3d_normalize(to);

    if (reb_vec3d_dot(from, to) >= 0) {  // small angle
        return reb_rotation_init_from_to_reduced(from, to);
    }

    //  More than 90 degrees apart, do rotation in two stages:
    //  (from -> half), (half -> to) 
    struct reb_vec3d half = {.x=from.x+to.x, .y=from.y+to.y, .z=from.z+to.z};
    half = reb_vec3d_normalize(half);

    if (reb_vec3d_length_squared(half) == 0) {
        //  half is nearly zero, so from and to point in nearly opposite directions
        //  and the rotation is numerically underspecified. Pick an axis orthogonal
        //  to the vectors, and use an angle of pi radians.
        struct reb_vec3d abs_from = {.x=fabs(from.x), .y=fabs(from.y), .z=fabs(from.z)};
        if (abs_from.x <= abs_from.y && abs_from.x <= abs_from.z){
            struct reb_vec3d axis = {.x=1, .y=0, .z = 0};
            axis = reb_vec3d_cross(from, axis);
            struct reb_rotation q = {.ix=axis.x, .iy=axis.y, .iz=axis.z, .r=0.0};
            return q;
        }
        if (abs_from.y <= abs_from.z){
            struct reb_vec3d axis = {.x=0, .y=1, .z = 0};
            axis = reb_vec3d_cross(from, axis);
            struct reb_rotation q = {.ix=axis.x, .iy=axis.y, .iz=axis.z, .r=0.0};
            return q;
        }
        struct reb_vec3d axis = {.x=0, .y=0, .z = 1};
        axis = reb_vec3d_cross(from, axis);
        struct reb_rotation q = {.ix=axis.x, .iy=axis.y, .iz=axis.z, .r=0.0};
        return q;
    }

    return reb_rotation_mul(reb_rotation_init_from_to_reduced(from, half), reb_rotation_init_from_to_reduced(half, to));
}

struct reb_rotation reb_rotation_init_angle_axis(const double angle, struct reb_vec3d axis){
    axis = reb_vec3d_normalize(axis);
    double cos2 = cos(angle/2.0);
    double sin2 = sin(angle/2.0);
    struct reb_vec3d imag = reb_vec3d_mul(axis, sin2);
    struct reb_rotation q = {.ix = imag.x, .iy = imag.y, .iz = imag.z, .r = cos2 };
    return q;
}

struct reb_rotation reb_rotation_init_to_new_axes(struct reb_vec3d newz, struct reb_vec3d newx){
    struct reb_vec3d z = {.x=0.0, .y=0.0, .z=1.0};
    struct reb_rotation q1 = reb_rotation_init_from_to(newz, z);
    struct reb_vec3d x = {.x=1.0, .y=0.0, .z=0.0};
    reb_vec3d_irotate(&newx, q1); // need to rotate newx to what it would be after the first rotation
    struct reb_rotation q2 = reb_rotation_init_from_to(newx, x);
    return reb_rotation_mul(q2, q1);
}

struct reb_rotation reb_rotation_init_orbital(const double Omega, const double inc, const double omega){
    // Murray and Dermot Eq. 2.121 (left hand side) 
    struct reb_vec3d x = {.x=1.0};
    struct reb_vec3d z = {.z=1.0};
    struct reb_rotation P1 = reb_rotation_init_angle_axis(omega, z);
    struct reb_rotation P2 = reb_rotation_init_angle_axis(inc, x);
    struct reb_rotation P3 = reb_rotation_init_angle_axis(Omega, z);
    return reb_rotation_mul(P3, reb_rotation_mul(P2, P1));
}

#define MIN_INC 1.e-8 
void reb_rotation_to_orbital(struct reb_rotation q, double* Omega, double* inc, double* omega){
    // see https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0276302
    // and https://github.com/evbernardes/quaternion_to_euler/blob/main/euler_from_rotation.py
    // Works but angles doen't always land in the right quadrant.
    double ap = q.r;
    double bp = q.iz;
    double cp = q.ix;
    double dp = q.iy;
    *inc = acos(2.0*(ap*ap+bp*bp) - 1.0);
    int safe1 =  (fabs(*inc) > MIN_INC);
    int safe2 =  (fabs(*inc - M_PI) > MIN_INC);

    if (safe1 && safe2){
        double half_sum = atan2(bp, ap);
        double half_diff = atan2(dp, cp);
        *omega = half_sum - half_diff;
        *Omega = half_sum + half_diff;
    }else{
        *Omega = 0;
        if (!safe1){
            double half_sum = atan2(bp, ap);
            *omega = 2.0 * half_sum;
        }else{
            double half_diff = atan2(dp, cp);
            *omega = 2.0 * half_diff;
        }
    }
    if (*omega < 0){
        *omega += M_PI*2.0;
    }
    if (*Omega < 0){
        *Omega += M_PI*2.0;
    }
}
