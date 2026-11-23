#ifndef STATION_H
#define STATION_H

#include <QString>
#include <QPointF>

class Station
{
public:
    Station(int id, const QString &name, double pos_km);

    int id() const { return s_id; }
    double pos() const { return s_pos_km; }
    QString name() const { return s_name; }

    void set_pos(const QPointF &p) { s_pos = p; }
    QPointF screen_pos() const { return s_pos; }

private:
    int s_id;
    QString s_name;
    double s_pos_km;
    QPointF s_pos;
};

#endif
