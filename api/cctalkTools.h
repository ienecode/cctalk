#ifndef CCTALKTOOLS_H
#define CCTALKTOOLS_H


#include <QString>
#include <QObject>
#include <QVector>
#include <QPair>
#include <QSerialPortInfo>

#include "cctalk/bill_validator_device.h"
#include "cctalk/coin_acceptor_device.h"
#include "appsettings.h"




/// Set up cctalk devices - coin acceptor and bill validator. This
/// reads the config file to get the device settings.
/// \return Error string or empty string if no error.
inline QString setUpCctalkDevices(CoinAcceptorDevice* coin_acceptor,
        const std::function<void(QString message)>& message_logger)
{
    // Note: If using multiple devices, all ccTalk options must be the same if the
    // device name is the same; except for cctalk_address, which must be
    // non-zero and must be different.

    auto coin_device = AppSettings::getValue<QString>(QStringLiteral("coin_acceptor/serial_device_name"));
    auto coin_cctalk_address = AppSettings::getValue<quint8>(QStringLiteral("coin_acceptor/cctalk_address"),
            ccCategoryGetDefaultAddress(CcCategory::CoinAcceptor));
    bool coin_des_encrypted = AppSettings::getValue<bool>(QStringLiteral("coin_acceptor/cctalk_des_encrypted"), false);
    bool coin_checksum_16bit = AppSettings::getValue<bool>(QStringLiteral("coin_acceptor/cctalk_checksum_16bit"), false);


    bool show_full_response = AppSettings::getValue<bool>("cctalk/show_full_response", false);
    bool show_serial_request = AppSettings::getValue<bool>("cctalk/show_serial_request", false);
    bool show_serial_response = AppSettings::getValue<bool>("cctalk/show_serial_response", false);
    bool show_cctalk_request = AppSettings::getValue<bool>("cctalk/show_cctalk_request", true);
    bool show_cctalk_response = AppSettings::getValue<bool>("cctalk/show_cctalk_response", true);



    // Coin acceptor
    if (coin_acceptor) {
        if (coin_device.isEmpty()) {
            return QObject::tr("! Coin acceptor configured device name is empty, cannot continue.");
        }
        message_logger(QObject::tr("* Coin acceptor configured device: %1").arg(coin_device));
        coin_acceptor->getLinkController().setCcTalkOptions(coin_device, coin_cctalk_address, coin_checksum_16bit, coin_des_encrypted);
        coin_acceptor->getLinkController().setLoggingOptions(show_full_response, show_serial_request, show_serial_response,
                show_cctalk_request, show_cctalk_response);

        QObject::connect(coin_acceptor, &CctalkDevice::logMessage, message_logger);
    }

    return QString();
}





/// Message accumulator. Helps with identifying duplicate messages.
struct MessageAccumulator {
    explicit MessageAccumulator(int buf_size) : buf(buf_size)
    { }

    inline int push(QString msg)
    {
        if (buf[index].first == msg) {
            ++(buf[index].second);
        } else {
            buf[index].first = std::move(msg);
            buf[index].second = 1;
        }
        index = (index+1) % buf.size();

        int min_repetitions = buf[index].second;
        for (const auto& p : buf) {
            min_repetitions = std::min(min_repetitions, p.second);
        }
        return min_repetitions;
    }

    QVector<QPair<QString, int>> buf;
    int index = 0;
};




/// Process string message from cctalk.
/// \return processed message
inline QString ccProcessLoggingMessage(QString msg, bool markup_output)
{
    static bool show_full_response = AppSettings::getValue<bool>("cctalk/show_full_response", false);
    static bool show_serial_request = AppSettings::getValue<bool>("cctalk/show_serial_request", false);
    static bool show_serial_response = AppSettings::getValue<bool>("cctalk/show_serial_response", false);
    static bool show_cctalk_request = AppSettings::getValue<bool>("cctalk/show_cctalk_request", true);
    static bool show_cctalk_response = AppSettings::getValue<bool>("cctalk/show_cctalk_response", true);

    bool show_msg = true;

    QString color = QStringLiteral("#000000");  // black
    if (show_full_response && msg.startsWith(QStringLiteral("< Full response:"))) {
        color = QStringLiteral("#c0c0c0");  // grey
    } else if (show_serial_response && msg.startsWith(QStringLiteral("< Response:"))) {
        color = QStringLiteral("#00A500");  // dark green
    } else if (show_cctalk_response && msg.startsWith(QStringLiteral("< ccTalk"))) {
        color = QStringLiteral("#00A597");  // marine
    } else if (show_serial_request && msg.startsWith(QStringLiteral("> Request:"))) {
        color = QStringLiteral("#7C65A5");  // violet
    } else if (show_cctalk_request && msg.startsWith(QStringLiteral("> ccTalk"))) {
        color = QStringLiteral("#5886A5");  // blue-grey
    } else if (msg.startsWith(QStringLiteral("* "))) {
        color = QStringLiteral("#B900CA");  // pink-violet
    } else if (msg.startsWith(QStringLiteral("! ")) || msg.startsWith(QStringLiteral("!<")) || msg.startsWith(QStringLiteral("!>"))) {
        color = QStringLiteral("#FF0000");  // red
    }

    static MessageAccumulator acc1(1), acc2(2), acc3(3), acc4(4);
    static int last_repeat_count = 0;

    const int max_shown_matches = 3;
    const int match_step = 40;
    int matches1 = acc1.push(msg);
    int matches2 = acc2.push(msg);
    int matches3 = acc3.push(msg);
// 	int matches4 = acc4.push(msg);

    if (matches1 > max_shown_matches) {
        if (matches1 % match_step == 0 && matches1 != last_repeat_count) {
            msg = QObject::tr("- The last message was repeated %1 times total").arg(matches1);
            last_repeat_count = matches1;
        } else {
            show_msg = false;
        }
    } else if (matches2 > max_shown_matches) {
        if (matches2 % match_step == 0 && matches2 != last_repeat_count) {
            msg = QObject::tr("- The last %1 messages were repeated %2 times total").arg(2).arg(matches2);
            last_repeat_count = matches2;
        } else {
            show_msg = false;
        }
    } else if (matches3 > max_shown_matches) {
        if (matches3 % match_step == 0 && matches3 != last_repeat_count) {
            msg = QObject::tr("- The last %1 messages were repeated %2 times total").arg(3).arg(matches3);
            last_repeat_count = matches3;
        } else {
            show_msg = false;
        }
// 	} else if (matches4 > max_shown_matches) {
// 		if (matches4 % match_step == 0 && matches4 != last_repeat_count) {
// 			msg = QObject::tr("- The last %1 messages were repeated %2 times total").arg(4).arg(matches4);
// 			last_repeat_count = matches4;
// 		} else {
// 			show_msg = false;
// 		}
    } else {
        last_repeat_count = 0;
    }

    // TODO If a new message arrives, print the remainder repeated messages.

    if (!show_msg) {
        return QString();
    }

    if (!markup_output) {
        return msg;
    }

  /*  QString formatted = msg.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br>"))
            .replace(QStringLiteral(" "), QStringLiteral("&#160;"));  // newline and nbsp

    QString style_str = QStringLiteral(" style=\"color: ") + color + QStringLiteral("\"");


    return QStringLiteral("<div") + style_str + QStringLiteral(">") + formatted + QStringLiteral("</div>");
    */
    return msg;
}



#endif // CCTALKTOOLS_H
