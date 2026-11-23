#include "train.h"
#include "route.h"
#include <algorithm>

Train::Train(int id, const Route *route)
    : t_id(id), t_route(route) {}

double Train::update(double cur_time, double delta, double segm_len, double speed)
{
    if (delta <= 1e-12) return 0.0;

    double time_left = delta;
    double current   = cur_time;
    const double speed_km_m = speed / 60.0;

    while (time_left > 1e-9) {
        switch (t_status) {
        case Train_Status::accident: {
            if (t_end_rep <= current) {
                t_status = Train_Status::move;
                break;
            }
            if (t_end_rep <= current + time_left) {
                time_left -= (t_end_rep - current);
                current    = t_end_rep;
                t_status   = Train_Status::move;
                t_accident_extra += (t_end_rep - current);
            } else {
                t_accident_extra += time_left;
                time_left = 0;
            }
            break;
        }
        case Train_Status::at_state: {
            if (!t_route) { time_left = 0; break; }
            if (t_cur_state + 1 >= t_route->stop_cnt()) {
                t_status = Train_Status::finished;
                time_left = 0;
                break;
            }

            double plan_dep = t_route->p_dep(t_cur_state) + t_delay + t_wait_delay;

            if (plan_dep <= current) {
                t_status = Train_Status::move;
            } else if (plan_dep <= current + time_left) {
                double dt_wait = plan_dep - current;
                time_left -= dt_wait;
                current   += dt_wait;
                t_status   = Train_Status::move;
            } else {
                time_left = 0;
            }
            break;
        }
        case Train_Status::move: {
            if (!t_route) { time_left = 0; break; }
            if (t_cur_state + 1 >= t_route->stop_cnt()) {
                t_status = Train_Status::finished;
                time_left = 0;
                break;
            }

            double dist_left = segm_len - t_pos_km;
            if (dist_left < 1e-9) { dist_left = segm_len; t_pos_km = 0; }
            double time_to_reach = dist_left / speed_km_m;

            if (time_to_reach <= time_left) {
                time_left -= time_to_reach;
                current   += time_to_reach;
                t_pos_km   = 0;
                t_cur_state++;
                t_status   = Train_Status::at_state;

                double planned_arr = t_route->p_arr(t_cur_state);
                t_delay = (current > planned_arr) ? (current - planned_arr) : 0.0;
                t_wait_delay = 0.0;
                t_accident_extra = 0.0;

                return delta - time_left;
            } else {
                t_pos_km += speed_km_m * time_left;
                time_left = 0;
            }
            break;
        }
        case Train_Status::wait:
        case Train_Status::finished:
            time_left = 0;
            break;
        }
    }
    return delta;
}

void Train::depart() {
    t_status = Train_Status::move;
    t_pos_km = 0;
}

void Train::arr_at_state(int s_id) {
    t_cur_state = s_id;
    t_pos_km = 0;
    t_status = Train_Status::at_state;
}

void Train::start_accident(double rep_end, double pos, double start_time) {
    t_status = Train_Status::accident;
    t_end_rep = rep_end;
    t_pos_km = pos;
    t_accident_start = start_time;
}

void Train::finish_rep() {
    t_status = Train_Status::move;
}

double Train::cal_act_delay() const {
    return t_delay + t_wait_delay + t_accident_extra;
}

double Train::estimated_arrival(int stop_index) const {
    if (!t_route) return 0.0;
    return t_route->p_arr(stop_index) + t_delay + t_wait_delay;
}
