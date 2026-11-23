#ifndef ROUTE_H
#define ROUTE_H

#include <QVector>

struct stop_inf {
    int s_id;
    double arr_time;
    double dep_time;
};

class Route
{
public:
    Route() = default;

    void add_stop(int s_id, double arr, double dep) {
        r_stops.append({s_id, arr, dep});
    }

    int stop_cnt() const { return r_stops.size(); }
    stop_inf stop(int i) const { return r_stops[i]; }
    double p_arr(int i) const { return r_stops[i].arr_time; }
    double p_dep(int i) const { return r_stops[i].dep_time; }

private:
    QVector<stop_inf> r_stops;
};

#endif
