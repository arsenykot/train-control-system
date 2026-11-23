#include "simulation.h"
#include <QTime>
#include <algorithm>
#include <cmath>

Simulation::Simulation()
{
    reset();
}

Simulation::~Simulation()
{
    qDeleteAll(m_routes);
    m_routes.clear();
}

int Simulation::random_int(int a, int b)
{
    std::uniform_int_distribution<int> dist(a, b);
    return dist(m_rng);
}

void Simulation::reset()
{
    qDeleteAll(m_routes);
    m_routes.clear();
    m_trains.clear();
    m_stations.clear();
    m_log.clear();
    m_blocked_tracks.clear();

    if (num_stations < 2)
        num_stations = 2;

    m_current_time = start_time;
    m_total_accident_duration = 0;
    m_total_accidents = 0;
    m_total_delay = 0;
    m_total_arrivals = 0;

    m_next_accident_time = start_time
                           + random_uniform(accident_interval_min, accident_interval_max);
    m_next_track_failure_time = start_time
                                + random_uniform(track_failure_interval_min,
                                                 track_failure_interval_max);

    for (int i = 0; i < num_stations; ++i) {
        m_stations.append(Station(i, "Станция " + QString::number(i + 1), i * segment_length_km));
    }

    double trav_time_min = (segment_length_km / speed_kmh) * 60.0;

    for (int i = 0; i < num_routes; ++i) {
        Route *r = new Route();
        int start_st = random_int(0, num_stations - 2);
        int end_st = random_int(start_st + 1, num_stations - 1);
        double dep_time = start_time + random_uniform(0, 360);
        double cur_time = dep_time;

        for (int j = start_st; j <= end_st; ++j) {
            double arr = cur_time;
            double dep = arr;
            if (j != start_st && j != end_st)
                dep = arr + random_uniform(2, 5);
            r->add_stop(j, arr, dep);
            cur_time = dep + trav_time_min;
        }

        m_routes.append(r);
        m_trains.append(Train(i, r));
    }
    add_log("Моделирование инициализировано");
}

double Simulation::random_uniform(double a, double b)
{
    std::uniform_real_distribution<double> dist(a, b);
    return dist(m_rng);
}

bool Simulation::random_chance(double p)
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(m_rng) < p;
}

void Simulation::add_log(const QString &text, double t)
{
    if (t < 0)
        t = m_current_time;
    int total_min = static_cast<int>(t);
    int h = total_min / 60;
    int m = total_min % 60;
    QString time_str = QString("%1:%2").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0'));
    m_log.append({QString("[%1] %2").arg(time_str, text)});
}

void Simulation::generate_events()
{
    if (m_current_time >= m_next_accident_time) {
        QVector<int> moving_trains;
        for (int i = 0; i < m_trains.size(); ++i) {
            if (m_trains[i].status() == Train_Status::move)
                moving_trains.append(i);
        }
        if (!moving_trains.isEmpty()) {
            int idx = moving_trains[random_int(0, moving_trains.size() - 1)];
            Train &t = m_trains[idx];

            double rep_end = m_current_time + random_uniform(repair_min, repair_max);
            t.start_accident(rep_end, t.pos_km(), m_current_time);
            add_log(QString("Поезд %1: АВАРИЯ на %2 км перегона")
                        .arg(t.id())
                        .arg(t.pos_km(), 0, 'f', 1),
                    m_current_time);
        }
        m_next_accident_time = m_current_time
                               + random_uniform(accident_interval_min, accident_interval_max);
    }

    if (m_current_time >= m_next_track_failure_time) {
        if (num_stations >= 2) {
            int start_st = random_int(0, num_stations - 2);
            int end_st = start_st + 1;
            double end_time = m_current_time + random_uniform(track_repair_min, track_repair_max);
            m_blocked_tracks.append({start_st, end_st, end_time});
            add_log(QString("Неисправность пути на перегоне %1-%2").arg(start_st + 1).arg(end_st + 1),
                    m_current_time);
        }
        m_next_track_failure_time = m_current_time
                                    + random_uniform(track_failure_interval_min,
                                                     track_failure_interval_max);
    }
}

void Simulation::clean_blocked_tracks()
{
    m_blocked_tracks.erase(std::remove_if(m_blocked_tracks.begin(),
                                          m_blocked_tracks.end(),
                                          [this](const TrackBlock &b) {
                                              return b.end_time <= m_current_time;
                                          }),
                           m_blocked_tracks.end());
}

void Simulation::compute_wait_delay(int i, double t)
{
    Train& tr = m_trains[i];
    if (tr.status() != Train_Status::at_state) return;

    int next_st_idx = tr.cur_state() + 1;
    if (next_st_idx >= m_routes[i]->stop_cnt()) return;

    int curr_s_id = m_routes[i]->stop(tr.cur_state()).s_id;
    int next_s_id = m_routes[i]->stop(next_st_idx).s_id;
    double plan_dep = m_routes[i]->p_dep(tr.cur_state()) + tr.delay();

    double max_end_time = 0;
    for (const auto& b : m_blocked_tracks) {
        if (b.start_st == curr_s_id && b.end_st == next_s_id && t < b.end_time)
            max_end_time = std::max(max_end_time, b.end_time);
    }
    double wait_failure = (max_end_time > plan_dep) ? (max_end_time - plan_dep) : 0.0;
    double wait_train = 0.0;
    const double v_min = speed_kmh / 60.0;
    const double L = segment_length_km;

    for (int j = 0; j < m_trains.size(); ++j) {
        if (j == i) continue;
        const Train& other = m_trains[j];

        int oc = other.cur_state();
        if (oc + 1 >= m_routes[j]->stop_cnt()) continue;
        int o_start = m_routes[j]->stop(oc).s_id;
        int o_end   = m_routes[j]->stop(oc + 1).s_id;
        if (o_start != curr_s_id || o_end != next_s_id) continue;
        const double t_ref = (j < i) ? (m_current_time + dt) : m_current_time;

        double exit_time = 0.0;
        bool blocks = false;

        if (other.status() == Train_Status::move) {
            double dist_left = L - other.pos_km();
            exit_time = t_ref + dist_left / v_min;
            blocks = true;
        } else if (other.status() == Train_Status::accident) {
            double dist_left = L - other.pos_km();
            exit_time = other.repair_end() + dist_left / v_min;
            blocks = true;
        } else if (other.status() == Train_Status::at_state) {
            double other_eff_dep = m_routes[j]->p_dep(oc)
            + other.delay()
                + other.wait_delay();
            if (other_eff_dep < plan_dep ||
                (other_eff_dep == plan_dep && j < i)) {
                exit_time = other_eff_dep + L / v_min;
                blocks = true;
            }
        }

        if (blocks && exit_time > plan_dep) {
            double w = exit_time - plan_dep;
            if (w > wait_train) wait_train = w;
        }
    }

    tr.set_wait_delay(std::max(wait_failure, wait_train));
}

void Simulation::roll_delay_up_to_current(int i, double t)
{
    Train &tr = m_trains[i];
    const int stops = m_routes[i]->stop_cnt();
    while (tr.last_delay_stop() < tr.cur_state() && tr.last_delay_stop() < stops - 2) {
        int next_stop = tr.last_delay_stop() + 1;
        tr.set_last_delay_stop(next_stop);
        if (random_chance(delay_prob)) {
            double d = random_uniform(delay_min, delay_max);
            tr.add_delay(d);
            add_log(QString("Поезд %1: задержка %2 мин на ст. %3")
                        .arg(tr.id())
                        .arg(d, 0, 'f', 1)
                        .arg(next_stop + 1),
                    t);
        }
    }
}

void Simulation::update_train(int i, double step_dt)
{
    Train &tr = m_trains[i];
    double remaining = step_dt;
    double t = m_current_time;
    int prev_state = tr.cur_state();

    roll_delay_up_to_current(i, t);

    while (remaining > 1e-9) {
        if (tr.status() == Train_Status::at_state) {
            compute_wait_delay(i, t);
        }

        double used = tr.update(t, remaining, segment_length_km, speed_kmh);

        if (tr.cur_state() != prev_state && tr.status() == Train_Status::at_state) {
            m_total_delay += tr.delay();
            ++m_total_arrivals;
            prev_state = tr.cur_state();
        }

        t += used;
        remaining -= used;

        if (tr.status() == Train_Status::finished)
            break;
        if (used < 1e-9)
            break;

        roll_delay_up_to_current(i, t);
    }
}

void Simulation::step()
{
    if (finished())
        return;

    clean_blocked_tracks();
    generate_events();

    QVector<Train_Status> prev_status(m_trains.size());
    for (int i = 0; i < m_trains.size(); ++i)
        prev_status[i] = m_trains[i].status();

    for (int i = 0; i < m_trains.size(); ++i)
        update_train(i, dt);

    for (int i = 0; i < m_trains.size(); ++i) {
        if (prev_status[i] == Train_Status::accident
            && m_trains[i].status() != Train_Status::accident) {
            double rep_end = m_trains[i].repair_end();
            m_total_accident_duration += (rep_end - m_trains[i].accident_start());
            ++m_total_accidents;
            add_log(QString("Поезд %1: ремонт завершён").arg(m_trains[i].id()), rep_end);
        }
    }

    m_current_time += dt;
}

double Simulation::get_average_delay() const
{
    if (m_total_arrivals == 0)
        return 0.0;
    return m_total_delay / m_total_arrivals;
}

double Simulation::get_total_accident_time() const
{
    return m_total_accident_duration;
}

void Simulation::add_custom_route(int start_st, int end_st, double dep_time)
{
    if (start_st < 0 || start_st >= num_stations)
        return;
    if (end_st <= start_st || end_st >= num_stations)
        return;
    if (dep_time < 0 || speed_kmh <= 0)
        return;

    Route *r = new Route();
    double cur_time = dep_time;
    double trav_time_min = (segment_length_km / speed_kmh) * 60.0;

    for (int j = start_st; j <= end_st; ++j) {
        double arr = cur_time;
        double dep = arr;
        if (j != start_st && j != end_st)
            dep = arr + random_uniform(2.0, 5.0);
        r->add_stop(j, arr, dep);
        cur_time = dep + trav_time_min;
    }
    m_routes.append(r);
    m_trains.append(Train(m_trains.size(), r));
    add_log(QString("Добавлен маршрут %1-%2").arg(start_st + 1).arg(end_st + 1));
}
