#ifndef SIMULATION_H
#define SIMULATION_H

#include <QVector>
#include <QString>
#include <random>
#include "station.h"
#include "route.h"
#include "train.h"

class Simulation
{
public:
    Simulation();
    ~Simulation();
    void reset();

    double current_time() const { return m_current_time; }
    bool finished() const { return m_current_time >= end_time; }

    int station_count() const { return m_stations.size(); }
    const Station& station(int i) const { return m_stations[i]; }

    int train_count() const { return m_trains.size(); }
    const Train& train(int i) const { return m_trains[i]; }

    void step();
    void set_seed(unsigned s) { m_rng.seed(s); }
    const Route& route(int i) const { return *m_routes[i]; }

    struct LogEntry { QString text; };
    const QVector<LogEntry>& log() const { return m_log; }

    void add_custom_route(int start_st, int end_st, double dep_time);
    double get_average_delay() const;
    double get_total_accident_time() const;

    double dt = 15.0;
    double start_time = 360.0;
    double end_time = 1440.0;
    int num_stations = 7;
    int num_routes = 7;
    double segment_length_km = 20.0;
    double speed_kmh = 70.0;

    double delay_prob = 0.10;
    double delay_min = 2.0;
    double delay_max = 10.0;

    double accident_interval_min = 120.0;
    double accident_interval_max = 900.0;
    double repair_min = 30.0;
    double repair_max = 120.0;

    double track_failure_interval_min = 120.0;
    double track_failure_interval_max = 900.0;
    double track_repair_min = 60.0;
    double track_repair_max = 180.0;

private:
    struct TrackBlock {
        int start_st;
        int end_st;
        double end_time;
    };

    QVector<Station> m_stations;
    QVector<Route*> m_routes;
    QVector<Train> m_trains;
    QVector<TrackBlock> m_blocked_tracks;

    double m_current_time = 0.0;
    QVector<LogEntry> m_log;

    std::mt19937 m_rng{std::random_device{}()};

    double m_next_accident_time = 0;
    double m_next_track_failure_time = 0;
    double m_total_accident_duration = 0;
    int m_total_accidents = 0;
    double m_total_delay = 0;
    int m_total_arrivals = 0;

    double random_uniform(double a, double b);
    bool random_chance(double p);
    int random_int(int a, int b);

    void add_log(const QString& text, double t = -1.0);
    void generate_events();
    void clean_blocked_tracks();
    void update_train(int i, double step_dt);
    void compute_wait_delay(int i, double t);
    void roll_delay_up_to_current(int i, double t);
};

#endif
