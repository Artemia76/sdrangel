///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 Edouard Griffiths, F4EXB                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <errno.h>

#include <QDebug>
#include <QNetworkReply>
#include <QBuffer>

#include "SWGDeviceSettings.h"
#include "SWGDeviceState.h"
#include "SWGDeviceReport.h"
#include "SWGLocalInputReport.h"

#include "util/simpleserializer.h"
#include "dsp/dspcommands.h"
#include "dsp/dspengine.h"
#include "device/devicesourceapi.h"
#include "dsp/filerecord.h"

#include "localinput.h"

MESSAGE_CLASS_DEFINITION(LocalInput::MsgConfigureLocalInput, Message)
MESSAGE_CLASS_DEFINITION(LocalInput::MsgFileRecord, Message)
MESSAGE_CLASS_DEFINITION(LocalInput::MsgStartStop, Message)
MESSAGE_CLASS_DEFINITION(LocalInput::MsgReportSampleRateAndFrequency, Message)

LocalInput::LocalInput(DeviceSourceAPI *deviceAPI) :
    m_deviceAPI(deviceAPI),
    m_settings(),
	m_deviceDescription("LocalInput"),
	m_startingTimeStamp(0)
{
	m_sampleFifo.setSize(96000 * 4);

    m_fileSink = new FileRecord(QString("test_%1.sdriq").arg(m_deviceAPI->getDeviceUID()));
    m_deviceAPI->addSink(m_fileSink);

    m_networkManager = new QNetworkAccessManager();
    connect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkManagerFinished(QNetworkReply*)));
}

LocalInput::~LocalInput()
{
    disconnect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkManagerFinished(QNetworkReply*)));
    delete m_networkManager;
	stop();
    m_deviceAPI->removeSink(m_fileSink);
    delete m_fileSink;
}

void LocalInput::destroy()
{
    delete this;
}

void LocalInput::init()
{
    applySettings(m_settings, true);
}

bool LocalInput::start()
{
	qDebug() << "LocalInput::start";
	return true;
}

void LocalInput::stop()
{
	qDebug() << "LocalInput::stop";
}

QByteArray LocalInput::serialize() const
{
    return m_settings.serialize();
}

bool LocalInput::deserialize(const QByteArray& data)
{
    bool success = true;

    if (!m_settings.deserialize(data))
    {
        m_settings.resetToDefaults();
        success = false;
    }

    MsgConfigureLocalInput* message = MsgConfigureLocalInput::create(m_settings, true);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue)
    {
        MsgConfigureLocalInput* messageToGUI = MsgConfigureLocalInput::create(m_settings, true);
        m_guiMessageQueue->push(messageToGUI);
    }

    return success;
}

void LocalInput::setMessageQueueToGUI(MessageQueue *queue)
{
    m_guiMessageQueue = queue;
}

const QString& LocalInput::getDeviceDescription() const
{
	return m_deviceDescription;
}

int LocalInput::getSampleRate() const
{
    return m_sampleRate;
}

void LocalInput::setSampleRate(int sampleRate)
{
    m_sampleRate = sampleRate;

    if (getMessageQueueToGUI())
    {
        MsgReportSampleRateAndFrequency *msg = MsgReportSampleRateAndFrequency::create(m_sampleRate, m_centerFrequency);
        getMessageQueueToGUI()->push(msg);
    }
}

quint64 LocalInput::getCenterFrequency() const
{
    return m_centerFrequency;
}

void LocalInput::setCenterFrequency(qint64 centerFrequency)
{
    m_centerFrequency = centerFrequency;

    if (getMessageQueueToGUI())
    {
        MsgReportSampleRateAndFrequency *msg = MsgReportSampleRateAndFrequency::create(m_sampleRate, m_centerFrequency);
        getMessageQueueToGUI()->push(msg);
    }
}

std::time_t LocalInput::getStartingTimeStamp() const
{
	return m_startingTimeStamp;
}

bool LocalInput::handleMessage(const Message& message)
{
    if (DSPSignalNotification::match(message))
    {
        DSPSignalNotification& notif = (DSPSignalNotification&) message;
        return m_fileSink->handleMessage(notif); // forward to file sink
    }
    else if (MsgFileRecord::match(message))
    {
        MsgFileRecord& conf = (MsgFileRecord&) message;
        qDebug() << "LocalInput::handleMessage: MsgFileRecord: " << conf.getStartStop();

        if (conf.getStartStop())
        {
            if (m_settings.m_fileRecordName.size() != 0) {
                m_fileSink->setFileName(m_settings.m_fileRecordName);
            } else {
                m_fileSink->genUniqueFileName(m_deviceAPI->getDeviceUID());
            }

            m_fileSink->startRecording();
        }
        else
        {
            m_fileSink->stopRecording();
        }

        return true;
    }
    else if (MsgStartStop::match(message))
    {
        MsgStartStop& cmd = (MsgStartStop&) message;
        qDebug() << "LocalInput::handleMessage: MsgStartStop: " << (cmd.getStartStop() ? "start" : "stop");

        if (cmd.getStartStop())
        {
            if (m_deviceAPI->initAcquisition())
            {
                m_deviceAPI->startAcquisition();
            }
        }
        else
        {
            m_deviceAPI->stopAcquisition();
        }

        if (m_settings.m_useReverseAPI) {
            webapiReverseSendStartStop(cmd.getStartStop());
        }

        return true;
    }
    else if (MsgConfigureLocalInput::match(message))
    {
        qDebug() << "LocalInput::handleMessage:" << message.getIdentifier();
        MsgConfigureLocalInput& conf = (MsgConfigureLocalInput&) message;
        applySettings(conf.getSettings(), conf.getForce());
        return true;
    }
	else
	{
		return false;
	}
}

void LocalInput::applySettings(const LocalInputSettings& settings, bool force)
{
    QMutexLocker mutexLocker(&m_mutex);
    std::ostringstream os;
    QString remoteAddress;
    QList<QString> reverseAPIKeys;

    if ((m_settings.m_dcBlock != settings.m_dcBlock) || force) {
        reverseAPIKeys.append("dcBlock");
    }
    if ((m_settings.m_iqCorrection != settings.m_iqCorrection) || force) {
        reverseAPIKeys.append("iqCorrection");
    }
    if ((m_settings.m_fileRecordName != settings.m_fileRecordName) || force) {
        reverseAPIKeys.append("fileRecordName");
    }

    if ((m_settings.m_dcBlock != settings.m_dcBlock) || (m_settings.m_iqCorrection != settings.m_iqCorrection) || force)
    {
        m_deviceAPI->configureCorrections(settings.m_dcBlock, settings.m_iqCorrection);
        qDebug("LocalInput::applySettings: corrections: DC block: %s IQ imbalance: %s",
                settings.m_dcBlock ? "true" : "false",
                settings.m_iqCorrection ? "true" : "false");
    }

    mutexLocker.unlock();

    if (settings.m_useReverseAPI)
    {
        bool fullUpdate = ((m_settings.m_useReverseAPI != settings.m_useReverseAPI) && settings.m_useReverseAPI) ||
                (m_settings.m_reverseAPIAddress != settings.m_reverseAPIAddress) ||
                (m_settings.m_reverseAPIPort != settings.m_reverseAPIPort) ||
                (m_settings.m_reverseAPIDeviceIndex != settings.m_reverseAPIDeviceIndex);
        webapiReverseSendSettings(reverseAPIKeys, settings, fullUpdate || force);
    }

    m_settings = settings;
    m_remoteAddress = remoteAddress;

    qDebug() << "LocalInput::applySettings: "
            << " m_dcBlock: " << m_settings.m_dcBlock
            << " m_iqCorrection: " << m_settings.m_iqCorrection
            << " m_fileRecordName: " << m_settings.m_fileRecordName
            << " m_remoteAddress: " << m_remoteAddress;
}

int LocalInput::webapiRunGet(
        SWGSDRangel::SWGDeviceState& response,
        QString& errorMessage)
{
    (void) errorMessage;
    m_deviceAPI->getDeviceEngineStateStr(*response.getState());
    return 200;
}

int LocalInput::webapiRun(
        bool run,
        SWGSDRangel::SWGDeviceState& response,
        QString& errorMessage)
{
    (void) errorMessage;
    m_deviceAPI->getDeviceEngineStateStr(*response.getState());
    MsgStartStop *message = MsgStartStop::create(run);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue) // forward to GUI if any
    {
        MsgStartStop *msgToGUI = MsgStartStop::create(run);
        m_guiMessageQueue->push(msgToGUI);
    }

    return 200;
}

int LocalInput::webapiSettingsGet(
                SWGSDRangel::SWGDeviceSettings& response,
                QString& errorMessage)
{
    (void) errorMessage;
    response.setLocalInputSettings(new SWGSDRangel::SWGLocalInputSettings());
    response.getLocalInputSettings()->init();
    webapiFormatDeviceSettings(response, m_settings);
    return 200;
}

int LocalInput::webapiSettingsPutPatch(
                bool force,
                const QStringList& deviceSettingsKeys,
                SWGSDRangel::SWGDeviceSettings& response, // query + response
                QString& errorMessage)
{
    (void) errorMessage;
    LocalInputSettings settings = m_settings;

    if (deviceSettingsKeys.contains("dcBlock")) {
        settings.m_dcBlock = response.getLocalInputSettings()->getDcBlock() != 0;
    }
    if (deviceSettingsKeys.contains("iqCorrection")) {
        settings.m_iqCorrection = response.getLocalInputSettings()->getIqCorrection() != 0;
    }
    if (deviceSettingsKeys.contains("fileRecordName")) {
        settings.m_fileRecordName = *response.getLocalInputSettings()->getFileRecordName();
    }
    if (deviceSettingsKeys.contains("useReverseAPI")) {
        settings.m_useReverseAPI = response.getLocalInputSettings()->getUseReverseApi() != 0;
    }
    if (deviceSettingsKeys.contains("reverseAPIAddress")) {
        settings.m_reverseAPIAddress = *response.getLocalInputSettings()->getReverseApiAddress();
    }
    if (deviceSettingsKeys.contains("reverseAPIPort")) {
        settings.m_reverseAPIPort = response.getLocalInputSettings()->getReverseApiPort();
    }
    if (deviceSettingsKeys.contains("reverseAPIDeviceIndex")) {
        settings.m_reverseAPIDeviceIndex = response.getLocalInputSettings()->getReverseApiDeviceIndex();
    }

    MsgConfigureLocalInput *msg = MsgConfigureLocalInput::create(settings, force);
    m_inputMessageQueue.push(msg);

    if (m_guiMessageQueue) // forward to GUI if any
    {
        MsgConfigureLocalInput *msgToGUI = MsgConfigureLocalInput::create(settings, force);
        m_guiMessageQueue->push(msgToGUI);
    }

    webapiFormatDeviceSettings(response, settings);
    return 200;
}

void LocalInput::webapiFormatDeviceSettings(SWGSDRangel::SWGDeviceSettings& response, const LocalInputSettings& settings)
{
    response.getLocalInputSettings()->setDcBlock(settings.m_dcBlock ? 1 : 0);
    response.getLocalInputSettings()->setIqCorrection(settings.m_iqCorrection);

    if (response.getLocalInputSettings()->getFileRecordName()) {
        *response.getLocalInputSettings()->getFileRecordName() = settings.m_fileRecordName;
    } else {
        response.getLocalInputSettings()->setFileRecordName(new QString(settings.m_fileRecordName));
    }

    response.getLocalInputSettings()->setUseReverseApi(settings.m_useReverseAPI ? 1 : 0);

    if (response.getLocalInputSettings()->getReverseApiAddress()) {
        *response.getLocalInputSettings()->getReverseApiAddress() = settings.m_reverseAPIAddress;
    } else {
        response.getLocalInputSettings()->setReverseApiAddress(new QString(settings.m_reverseAPIAddress));
    }

    response.getLocalInputSettings()->setReverseApiPort(settings.m_reverseAPIPort);
    response.getLocalInputSettings()->setReverseApiDeviceIndex(settings.m_reverseAPIDeviceIndex);
}

int LocalInput::webapiReportGet(
        SWGSDRangel::SWGDeviceReport& response,
        QString& errorMessage)
{
    (void) errorMessage;
    response.setLocalInputReport(new SWGSDRangel::SWGLocalInputReport());
    response.getLocalInputReport()->init();
    webapiFormatDeviceReport(response);
    return 200;
}

void LocalInput::webapiFormatDeviceReport(SWGSDRangel::SWGDeviceReport& response)
{
    response.getLocalInputReport()->setCenterFrequency(m_centerFrequency);
    response.getLocalInputReport()->setSampleRate(m_sampleRate);
}

void LocalInput::webapiReverseSendSettings(QList<QString>& deviceSettingsKeys, const LocalInputSettings& settings, bool force)
{
    SWGSDRangel::SWGDeviceSettings *swgDeviceSettings = new SWGSDRangel::SWGDeviceSettings();
    swgDeviceSettings->setTx(0);
    swgDeviceSettings->setOriginatorIndex(m_deviceAPI->getDeviceSetIndex());
    swgDeviceSettings->setDeviceHwType(new QString("LocalInput"));
    swgDeviceSettings->setLocalInputSettings(new SWGSDRangel::SWGLocalInputSettings());
    SWGSDRangel::SWGLocalInputSettings *swgLocalInputSettings = swgDeviceSettings->getLocalInputSettings();

    // transfer data that has been modified. When force is on transfer all data except reverse API data

    if (deviceSettingsKeys.contains("dcBlock") || force) {
        swgLocalInputSettings->setDcBlock(settings.m_dcBlock ? 1 : 0);
    }
    if (deviceSettingsKeys.contains("iqCorrection") || force) {
        swgLocalInputSettings->setIqCorrection(settings.m_iqCorrection ? 1 : 0);
    }
    if (deviceSettingsKeys.contains("fileRecordName") || force) {
        swgLocalInputSettings->setFileRecordName(new QString(settings.m_fileRecordName));
    }

    QString deviceSettingsURL = QString("http://%1:%2/sdrangel/deviceset/%3/device/settings")
            .arg(settings.m_reverseAPIAddress)
            .arg(settings.m_reverseAPIPort)
            .arg(settings.m_reverseAPIDeviceIndex);
    m_networkRequest.setUrl(QUrl(deviceSettingsURL));
    m_networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QBuffer *buffer=new QBuffer();
    buffer->open((QBuffer::ReadWrite));
    buffer->write(swgDeviceSettings->asJson().toUtf8());
    buffer->seek(0);

    // Always use PATCH to avoid passing reverse API settings
    m_networkManager->sendCustomRequest(m_networkRequest, "PATCH", buffer);

    delete swgDeviceSettings;
}

void LocalInput::webapiReverseSendStartStop(bool start)
{
    SWGSDRangel::SWGDeviceSettings *swgDeviceSettings = new SWGSDRangel::SWGDeviceSettings();
    swgDeviceSettings->setTx(0);
    swgDeviceSettings->setOriginatorIndex(m_deviceAPI->getDeviceSetIndex());
    swgDeviceSettings->setDeviceHwType(new QString("LocalInput"));

    QString deviceSettingsURL = QString("http://%1:%2/sdrangel/deviceset/%3/device/run")
            .arg(m_settings.m_reverseAPIAddress)
            .arg(m_settings.m_reverseAPIPort)
            .arg(m_settings.m_reverseAPIDeviceIndex);
    m_networkRequest.setUrl(QUrl(deviceSettingsURL));
    m_networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QBuffer *buffer=new QBuffer();
    buffer->open((QBuffer::ReadWrite));
    buffer->write(swgDeviceSettings->asJson().toUtf8());
    buffer->seek(0);

    if (start) {
        m_networkManager->sendCustomRequest(m_networkRequest, "POST", buffer);
    } else {
        m_networkManager->sendCustomRequest(m_networkRequest, "DELETE", buffer);
    }
}

void LocalInput::networkManagerFinished(QNetworkReply *reply)
{
    QNetworkReply::NetworkError replyError = reply->error();

    if (replyError)
    {
        qWarning() << "LocalInput::networkManagerFinished:"
                << " error(" << (int) replyError
                << "): " << replyError
                << ": " << reply->errorString();
        return;
    }

    QString answer = reply->readAll();
    answer.chop(1); // remove last \n
    qDebug("LocalInput::networkManagerFinished: reply:\n%s", answer.toStdString().c_str());
}
