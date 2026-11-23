#ifndef TRAIN_H
#define TRAIN_H

class Route;

enum class Train_Status { at_state, move, accident, wait, finished};

class Train
{
public:
    Train(int id, const Route *route);

    int id() const { return t_id; }
    Train_Status status() const { return t_status; }
    int cur_state() const { return t_cur_state; }
    double pos_km() const { return t_pos_km; }
    double delay() const { return t_delay; }
    double repair_end() const { return t_end_rep; }

    double update(double cur_time, double delta, double segm_len, double speed);

    void depart();
    void arr_at_state(int s_id);
    void start_accident(double rep_end, double pos, double start_time);
    void finish_rep();
    void add_delay(double min) { t_delay += min; }
    double cal_act_delay() const;
    int  last_delay_stop() const { return t_last_delay_stop; }
    void set_last_delay_stop(int s) { t_last_delay_stop = s; }

    double accident_start() const { return t_accident_start; }
    double wait_delay() const { return t_wait_delay; }
    void set_wait_delay(double d) { t_wait_delay = d > 0 ? d : 0; }

    double estimated_arrival(int stop_index) const;

private:
    int t_id;
    const Route *t_route = nullptr;

    Train_Status t_status = Train_Status::at_state;
    int t_cur_state = 0;
    double t_pos_km = 0;
    double t_delay = 0;
    double t_end_rep = 0;
    int t_last_delay_stop = -1;
    double t_accident_start = 0;
    double t_wait_delay = 0;
    double t_accident_extra = 0.0;
};

#endif
