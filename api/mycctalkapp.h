#ifndef MYCCTALKAPP_H
#define MYCCTALKAPP_H


#include <QCloseEvent>
#include <QScopedPointer>
#include <QDebug>

#include "cctalk/coin_acceptor_device.h"


class MyCcTalkApp: public QObject
{
    Q_OBJECT
public:
    explicit MyCcTalkApp(QObject *parent = nullptr);

public slots:

    /// Launch device-handling threads
    void runSerialThreads();

    /// Log message to message log
    void logMessage(QString msg);

    /// Button click callback
    void onStartStopCoinAcceptor();

    /// Button click callback
    void onToggleCoinAccept();



private:


    qtcc::CoinAcceptorDevice coin_acceptor_;  ///< Coin acceptor communicator 

    QTimer timerRefresh;
};


#endif // MYCCTALKAPP_H
