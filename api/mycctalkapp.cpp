#include <QTimer>
#include <Qt>
#include <QSerialPortInfo>
#include <QPointer>
#include <cmath>

#include "cctalk/helpers/debug.h"
#include "appsettings.h"
#include "mycctalkapp.h"
#include "cctalkTools.h"


//using CcDeviceState;

MyCcTalkApp::MyCcTalkApp(QObject *parent)
    : QObject{parent}
{
    // Load application settings
    AppSettings::init();

    QTimer::singleShot(0, this, SLOT(runSerialThreads()));

    QTimer::singleShot(10000, this, &MyCcTalkApp::onStartStopCoinAcceptor);
    QTimer::singleShot(20000, this, &MyCcTalkApp::onToggleCoinAccept);

}

void MyCcTalkApp::runSerialThreads()
{
    // Set cctalk options
    QString setup_error = setUpCctalkDevices(&coin_acceptor_, [=](QString message) {
        logMessage(message);
    });
    if (!setup_error.isEmpty()) {
        logMessage(setup_error);
        return;
    }
    // Coin acceptor
    {
        connect(&coin_acceptor_, &CctalkDevice::creditAccepted, [this]([[maybe_unused]] quint8 id, const CcIdentifier& identifier) {
            const char* prop_name = "integral_value";
            quint64 divisor = 1;
            quint64 value = identifier.getValue(divisor);

            qDebug() << "CreditAccepted: " << value;

        });
    }


}

void MyCcTalkApp::logMessage(QString msg)
{

    msg = ccProcessLoggingMessage(msg, true);
    if (!msg.isEmpty()) {
        qDebug() << msg;
    }

}



void MyCcTalkApp::onStartStopCoinAcceptor()
{

    if (coin_acceptor_.getDeviceState() == CcDeviceState::ShutDown) {

        coin_acceptor_.getLinkController().openPort([this](const QString& error_msg) {

            if (error_msg.isEmpty()) {
                coin_acceptor_.initialize([]([[maybe_unused]] const QString& init_error_msg) { });
            }

        });
    }
    else
    {

        coin_acceptor_.shutdown([this]([[maybe_unused]] const QString& error_msg) {
            // Close the port once the device is "shut down"
            coin_acceptor_.getLinkController().closePort();
        });
    }
}
void MyCcTalkApp::onToggleCoinAccept()
{
    bool accepting = coin_acceptor_.getDeviceState() == CcDeviceState::NormalAccepting;
    bool rejecting = coin_acceptor_.getDeviceState() == CcDeviceState::NormalRejecting;
    if (accepting || rejecting) {
        CcDeviceState new_state = accepting ? CcDeviceState::NormalRejecting : CcDeviceState::NormalAccepting;
        coin_acceptor_.requestSwitchDeviceState(new_state, []([[maybe_unused]] const QString& error_msg) {
            // nothing
        });
    } else {
        logMessage(tr("! Cannot toggle coin accept mode, the device is in %1 state.")
                   .arg(ccDeviceStateGetDisplayableName(coin_acceptor_.getDeviceState())));
    }

}
