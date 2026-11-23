#include <QCoreApplication>
#include <QDebug>
#include <QTime>
#include <QSet>
#include <cmath>
#include <algorithm>
#include "simulation.h"

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, msg) do {                                              \
if (cond) { qInfo().noquote() << "    OK  " << msg; ++g_passed; }      \
    else      { qWarning().noquote() << "    FAIL " << msg; ++g_failed; }  \
} while (0)

#define CHECK_NEAR(a, b, eps, msg) do {                                    \
    if (std::fabs((a) - (b)) <= (eps)) {                                   \
            qInfo().noquote() << "    OK  " << msg; ++g_passed;                \
    } else {                                                               \
            qWarning().noquote() << "    FAIL " << msg                         \
            << "(got" << (a) << ", expected" << (b) << ")";\
            ++g_failed;                                                        \
    }                                                                      \
} while (0)

    static QString fmt(double min) {
        return QTime(0, 0).addSecs(int(min * 60)).toString("HH:mm");
    }

// Обратите внимание: вместо accident_prob теперь enable_accidents (bool)
static void setup(Simulation& sim, double dt, double delay_prob, bool enable_accidents, unsigned seed = 42)
{
    sim.dt                = dt;
    sim.start_time        = 360.0;
    sim.end_time          = 900.0;
    sim.num_stations      = 7;
    sim.num_routes        = 7;
    sim.segment_length_km = 20.0;
    sim.speed_kmh         = 70.0;
    sim.delay_prob        = delay_prob;
    sim.delay_min         = 2.0;
    sim.delay_max         = 10.0;

    if (enable_accidents) {
        sim.accident_interval_min = 10.0;
        sim.accident_interval_max = 30.0;
    } else {
        sim.accident_interval_min = 100000.0;
        sim.accident_interval_max = 200000.0;
    }
    sim.repair_min        = 30.0;
    sim.repair_max        = 120.0;

    sim.track_failure_interval_min = 100000.0;
    sim.track_failure_interval_max = 200000.0;

    sim.set_seed(seed);
    sim.reset();
}

struct Stats {
    int    arrivals         = 0;
    double sum_delay        = 0.0;
    double max_delay        = 0.0;
    int    on_time          = 0;
    int    accidents        = 0;
    int    delay_log_entries= 0;
    int    repair_log_entries=0;
};

static Stats run_with_stats(Simulation& sim)
{
    Stats st;
    const int n = sim.train_count();
    QVector<int> last_state(n, -1);
    QVector<Train_Status> last_status(n, Train_Status::wait);

    while (!sim.finished()) {
        sim.step();
        for (int i = 0; i < n; ++i) {
            const Train& t = sim.train(i);
            if (t.status() == Train_Status::at_state && t.cur_state() != last_state[i]) {
                ++st.arrivals;
                double d = t.cal_act_delay();
                st.sum_delay += d;
                if (d > st.max_delay) st.max_delay = d;
                if (d < 1.0) ++st.on_time;
            }
            if (t.status() == Train_Status::accident && last_status[i] != Train_Status::accident) {
                ++st.accidents;
            }
            last_state[i]  = t.cur_state();
            last_status[i] = t.status();
        }
    }
    for (const auto& e : sim.log()) {
        if (e.text.contains("задерж")) ++st.delay_log_entries;
        if (e.text.contains("ремонт завершён")) ++st.repair_log_entries;
    }
    return st;
}

static void test_schedule_generation()
{
    qInfo().noquote() << "\n[1] Initial schedule properties";
    Simulation sim;
    setup(sim, 15.0, 0.0, false);
    sim.num_routes = 5;
    sim.reset();

    const Route& r = sim.route(0);
    double trav = sim.segment_length_km / sim.speed_kmh * 60.0;

    CHECK(r.stop_cnt() >= 2, "route has at least 2 stops");
    CHECK_NEAR(r.p_arr(0), r.p_dep(0), 1e-6, "departure from first stop == arrival");
    CHECK_NEAR(r.p_arr(1), r.p_dep(0) + trav, 1e-6, "arrival at 2nd stop == dep_0 + travel_time");

    double dwell = r.p_dep(1) - r.p_arr(1);
    if (r.stop_cnt() > 2) {
        CHECK(dwell >= 2.0 && dwell <= 5.0, "dwell time is between 2 and 5 mins");
    }
}

static void test_time_step_is_exact()
{
    qInfo().noquote() << "\n[2] Each step advances model time by exactly dt";
    Simulation sim;
    setup(sim, 15.0, 0.0, false);

    for (int k = 0; k < 5 && !sim.finished(); ++k) {
        const double prev = sim.current_time();
        sim.step();
        CHECK_NEAR(sim.current_time() - prev, sim.dt, 1e-9, QString("step %1").arg(k + 1));
    }
}

static void test_no_spurious_delays()
{
    qInfo().noquote() << "\n[3] No events -> no delays (dt = 15 and 30)";
    for (double dt : {15.0, 30.0}) {
        Simulation sim;
        setup(sim, dt, 0.0, false);
        Stats st = run_with_stats(sim);

        CHECK(st.arrivals > 0, "there were arrivals");
        CHECK(st.accidents == 0, "no accidents");
        CHECK_NEAR(st.max_delay, 0.0, 1e-6, "max delay = 0");
    }
}

static void test_delay_once_per_stop()
{
    qInfo().noquote() << "\n[4] Delay is rolled once per stop";
    int prev_entries = -1;
    for (double dt : {15.0, 10.0, 5.0}) {
        Simulation sim;
        setup(sim, dt, 1.0, false);
        Stats st = run_with_stats(sim);
        if (prev_entries >= 0) {
            CHECK(st.delay_log_entries == prev_entries, "same number of delay rolls");
        }
        prev_entries = st.delay_log_entries;
    }
}

static void test_delay_propagation()
{
    qInfo().noquote() << "\n[5] Generated delays accumulate";
    Simulation sim;
    setup(sim, 15.0, 0.5, false);
    Stats st = run_with_stats(sim);

    CHECK(st.arrivals > 0, "there were arrivals");
    CHECK(st.sum_delay > 0.0, "sum delay > 0");
    CHECK(st.max_delay >= sim.delay_min, "at least one delay >= delay_min");
}

static void test_accident_handling()
{
    qInfo().noquote() << "\n[6] Accident: train stops, repairs, finishes";
    Simulation sim;
    setup(sim, 15.0, 0.0, true);
    sim.end_time = 1440.0;
    Stats st = run_with_stats(sim);

    CHECK(st.accidents >= 1, "at least one accident happened");
    CHECK(st.repair_log_entries >= st.accidents, "repairs logged");
}

static void test_log_contains_key_events()
{
    qInfo().noquote() << "\n[9] Log contains key events";
    Simulation sim;
    setup(sim, 15.0, 0.0, true);
    sim.end_time = 1440.0;
    run_with_stats(sim);

    bool has_init = false, has_repair = false, has_accident = false;
    for (const auto& e : sim.log()) {
        if (e.text.contains("инициализировано")) has_init = true;
        if (e.text.contains("ремонт завершён"))   has_repair = true;
        if (e.text.contains("АВАРИЯ"))            has_accident = true;
    }
    CHECK(has_init,     "initialization present");
    CHECK(has_accident, "accident present");
    CHECK(has_repair,   "repair present");
}

static void test_track_failure()
{
    qInfo().noquote() << "\n[11] Track failure blocks trains";
    Simulation sim;
    setup(sim, 15.0, 0.0, false);
    sim.track_failure_interval_min = 10.0;
    sim.track_failure_interval_max = 20.0;
    sim.reset();
    run_with_stats(sim);

    bool has_track_fail = false;
    for (const auto& e : sim.log()) {
        if (e.text.contains("Неисправность пути")) has_track_fail = true;
    }
    CHECK(has_track_fail, "track failure event generated");
}

static void test_custom_route()
{
    qInfo().noquote() << "\n[12] Custom route addition";
    Simulation sim;
    setup(sim, 15.0, 0.0, false);
    int initial_trains = sim.train_count();
    sim.add_custom_route(1, 4, 480.0);

    CHECK(sim.train_count() == initial_trains + 1, "train count increased");
    const Route& r = sim.route(initial_trains);
    CHECK(r.stop_cnt() == 4, "route has 4 stops");
    CHECK_NEAR(r.p_dep(0), 480.0, 1e-6, "departure time matches");
}

static void test_stats()
{
    qInfo().noquote() << "\n[13] Statistics collection";
    Simulation sim;
    setup(sim, 15.0, 0.5, true);
    sim.end_time = 1440.0;
    run_with_stats(sim);

    CHECK(sim.get_average_delay() >= 0.0, "average delay is valid");
    CHECK(sim.get_total_accident_time() >= 0.0, "total accident time is valid");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    qInfo().noquote() << "=== TRAIN TRAFFIC CONTROL: FULL BACKEND TESTS ===";

    test_schedule_generation();
    test_time_step_is_exact();
    test_no_spurious_delays();
    test_delay_once_per_stop();
    test_delay_propagation();
    test_accident_handling();
    test_log_contains_key_events();
    test_track_failure();
    test_custom_route();
    test_stats();

    qInfo().noquote() << "\n=== TOTAL ===";
    qInfo().noquote() << "passed:" << g_passed << ", failed:" << g_failed;
    return g_failed == 0 ? 0 : 1;
}
