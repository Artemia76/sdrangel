///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 Edouard Griffiths, F4EXB                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <QPixmap>

#include "freedvdemodgui.h"

#include "device/devicesourceapi.h"
#include "device/deviceuiset.h"

#include "ui_freedvdemodgui.h"
#include "dsp/spectrumvis.h"
#include "dsp/dspengine.h"
#include "dsp/dspcommands.h"
#include "gui/glspectrum.h"
#include "gui/basicchannelsettingsdialog.h"
#include "plugin/pluginapi.h"
#include "util/simpleserializer.h"
#include "util/db.h"
#include "gui/crightclickenabler.h"
#include "gui/audioselectdialog.h"
#include "mainwindow.h"
#include "freedvdemod.h"

FreeDVDemodGUI* FreeDVDemodGUI::create(PluginAPI* pluginAPI, DeviceUISet *deviceUISet, BasebandSampleSink *rxChannel)
{
    FreeDVDemodGUI* gui = new FreeDVDemodGUI(pluginAPI, deviceUISet, rxChannel);
	return gui;
}

void FreeDVDemodGUI::destroy()
{
	delete this;
}

void FreeDVDemodGUI::setName(const QString& name)
{
	setObjectName(name);
}

QString FreeDVDemodGUI::getName() const
{
	return objectName();
}

qint64 FreeDVDemodGUI::getCenterFrequency() const
{
	return m_channelMarker.getCenterFrequency();
}

void FreeDVDemodGUI::setCenterFrequency(qint64 centerFrequency)
{
	m_channelMarker.setCenterFrequency(centerFrequency);
	m_settings.m_inputFrequencyOffset = m_channelMarker.getCenterFrequency();
	applySettings();
}

void FreeDVDemodGUI::resetToDefaults()
{
	m_settings.resetToDefaults();
}

QByteArray FreeDVDemodGUI::serialize() const
{
    return m_settings.serialize();
}

bool FreeDVDemodGUI::deserialize(const QByteArray& data)
{
    if(m_settings.deserialize(data))
    {
        displaySettings();
        applySettings(true); // will have true
        return true;
    }
    else
    {
        m_settings.resetToDefaults();
        displaySettings();
        applySettings(true); // will have true
        return false;
    }
}

bool FreeDVDemodGUI::handleMessage(const Message& message)
{
    if (FreeDVDemod::MsgConfigureFreeDVDemod::match(message))
    {
        qDebug("FreeDVDemodGUI::handleMessage: FreeDVDemodGUI::MsgConfigureFreeDVDemod");
        const FreeDVDemod::MsgConfigureFreeDVDemod& cfg = (FreeDVDemod::MsgConfigureFreeDVDemod&) message;
        m_settings = cfg.getSettings();
        blockApplySettings(true);
        displaySettings();
        blockApplySettings(false);
        return true;
    }
    else if (DSPConfigureAudio::match(message))
    {
        qDebug("FreeDVDemodGUI::handleMessage: DSPConfigureAudio: %d", m_freeDVDemod->getAudioSampleRate());
        applyBandwidths(5 - ui->spanLog2->value()); // will update spectrum details with new sample rate
        return true;
    }
    else
    {
        return false;
    }
}

void FreeDVDemodGUI::handleInputMessages()
{
    Message* message;

    while ((message = getInputMessageQueue()->pop()) != 0)
    {
        if (handleMessage(*message))
        {
            delete message;
        }
    }
}

void FreeDVDemodGUI::channelMarkerChangedByCursor()
{
    ui->deltaFrequency->setValue(m_channelMarker.getCenterFrequency());
    m_settings.m_inputFrequencyOffset = m_channelMarker.getCenterFrequency();
    applySettings();
}

void FreeDVDemodGUI::channelMarkerHighlightedByCursor()
{
    setHighlighted(m_channelMarker.getHighlighted());
}

void FreeDVDemodGUI::on_audioBinaural_toggled(bool binaural)
{
	m_audioBinaural = binaural;
	m_settings.m_audioBinaural = binaural;
	applySettings();
}

void FreeDVDemodGUI::on_audioFlipChannels_toggled(bool flip)
{
	m_audioFlipChannels = flip;
	m_settings.m_audioFlipChannels = flip;
	applySettings();
}

void FreeDVDemodGUI::on_dsb_toggled(bool dsb)
{
    ui->flipSidebands->setEnabled(!dsb);
    applyBandwidths(5 - ui->spanLog2->value());
}

void FreeDVDemodGUI::on_deltaFrequency_changed(qint64 value)
{
    m_channelMarker.setCenterFrequency(value);
    m_settings.m_inputFrequencyOffset = m_channelMarker.getCenterFrequency();
    applySettings();
}

void FreeDVDemodGUI::on_BW_valueChanged(int value)
{
    (void) value;
    applyBandwidths(5 - ui->spanLog2->value());
}

void FreeDVDemodGUI::on_lowCut_valueChanged(int value)
{
    (void) value;
    applyBandwidths(5 - ui->spanLog2->value());
}

void FreeDVDemodGUI::on_volume_valueChanged(int value)
{
	ui->volumeText->setText(QString("%1").arg(value / 10.0, 0, 'f', 1));
	m_settings.m_volume = value / 10.0;
	applySettings();
}

void FreeDVDemodGUI::on_agc_toggled(bool checked)
{
    m_settings.m_agc = checked;
    applySettings();
}

void FreeDVDemodGUI::on_agcClamping_toggled(bool checked)
{
    m_settings.m_agcClamping = checked;
    applySettings();
}

void FreeDVDemodGUI::on_agcTimeLog2_valueChanged(int value)
{
    QString s = QString::number((1<<value), 'f', 0);
    ui->agcTimeText->setText(s);
    m_settings.m_agcTimeLog2 = value;
    applySettings();
}

void FreeDVDemodGUI::on_agcPowerThreshold_valueChanged(int value)
{
    displayAGCPowerThreshold(value);
    m_settings.m_agcPowerThreshold = value;
    applySettings();
}

void FreeDVDemodGUI::on_agcThresholdGate_valueChanged(int value)
{
    QString s = QString::number(value, 'f', 0);
    ui->agcThresholdGateText->setText(s);
    m_settings.m_agcThresholdGate = value;
    applySettings();
}

void FreeDVDemodGUI::on_audioMute_toggled(bool checked)
{
	m_audioMute = checked;
	m_settings.m_audioMute = checked;
	applySettings();
}

void FreeDVDemodGUI::on_spanLog2_valueChanged(int value)
{
    if ((value < 0) || (value > 4)) {
        return;
    }

    applyBandwidths(5 - ui->spanLog2->value());
}

void FreeDVDemodGUI::on_flipSidebands_clicked(bool checked)
{
    (void) checked;
    int bwValue = ui->BW->value();
    int lcValue = ui->lowCut->value();
    ui->BW->setValue(-bwValue);
    ui->lowCut->setValue(-lcValue);
}

void FreeDVDemodGUI::onMenuDialogCalled(const QPoint &p)
{
    BasicChannelSettingsDialog dialog(&m_channelMarker, this);
    dialog.setUseReverseAPI(m_settings.m_useReverseAPI);
    dialog.setReverseAPIAddress(m_settings.m_reverseAPIAddress);
    dialog.setReverseAPIPort(m_settings.m_reverseAPIPort);
    dialog.setReverseAPIDeviceIndex(m_settings.m_reverseAPIDeviceIndex);
    dialog.setReverseAPIChannelIndex(m_settings.m_reverseAPIChannelIndex);

    dialog.move(p);
    dialog.exec();

    m_settings.m_inputFrequencyOffset = m_channelMarker.getCenterFrequency();
    m_settings.m_rgbColor = m_channelMarker.getColor().rgb();
    m_settings.m_title = m_channelMarker.getTitle();
    m_settings.m_useReverseAPI = dialog.useReverseAPI();
    m_settings.m_reverseAPIAddress = dialog.getReverseAPIAddress();
    m_settings.m_reverseAPIPort = dialog.getReverseAPIPort();
    m_settings.m_reverseAPIDeviceIndex = dialog.getReverseAPIDeviceIndex();
    m_settings.m_reverseAPIChannelIndex = dialog.getReverseAPIChannelIndex();

    setWindowTitle(m_settings.m_title);
    setTitleColor(m_settings.m_rgbColor);

    applySettings();
}

void FreeDVDemodGUI::onWidgetRolled(QWidget* widget, bool rollDown)
{
    (void) widget;
    (void) rollDown;
}

FreeDVDemodGUI::FreeDVDemodGUI(PluginAPI* pluginAPI, DeviceUISet *deviceUISet, BasebandSampleSink *rxChannel, QWidget* parent) :
	RollupWidget(parent),
	ui(new Ui::FreeDVDemodGUI),
	m_pluginAPI(pluginAPI),
	m_deviceUISet(deviceUISet),
	m_channelMarker(this),
	m_doApplySettings(true),
    m_spectrumRate(6000),
	m_audioBinaural(false),
	m_audioFlipChannels(false),
    m_audioMute(false),
	m_squelchOpen(false)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	connect(this, SIGNAL(widgetRolled(QWidget*,bool)), this, SLOT(onWidgetRolled(QWidget*,bool)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onMenuDialogCalled(const QPoint &)));

	m_spectrumVis = new SpectrumVis(SDR_RX_SCALEF, ui->glSpectrum);
	m_freeDVDemod = (FreeDVDemod*) rxChannel;
	m_freeDVDemod->setSampleSink(m_spectrumVis);
	m_freeDVDemod->setMessageQueueToGUI(getInputMessageQueue());

	resetToDefaults();

    ui->glSpectrum->setCenterFrequency(m_spectrumRate/2);
    ui->glSpectrum->setSampleRate(m_spectrumRate);
	ui->glSpectrum->setDisplayWaterfall(true);
	ui->glSpectrum->setDisplayMaxHold(true);
    ui->glSpectrum->setSsbSpectrum(true);
	ui->glSpectrum->connectTimer(MainWindow::getInstance()->getMasterTimer());

	connect(&MainWindow::getInstance()->getMasterTimer(), SIGNAL(timeout()), this, SLOT(tick()));

	CRightClickEnabler *audioMuteRightClickEnabler = new CRightClickEnabler(ui->audioMute);
    connect(audioMuteRightClickEnabler, SIGNAL(rightClick(const QPoint &)), this, SLOT(audioSelect()));

    ui->deltaFrequencyLabel->setText(QString("%1f").arg(QChar(0x94, 0x03)));
    ui->deltaFrequency->setColorMapper(ColorMapper(ColorMapper::GrayGold));
    ui->deltaFrequency->setValueRange(false, 7, -9999999, 9999999);
	ui->channelPowerMeter->setColorTheme(LevelMeterSignalDB::ColorGreenAndBlue);

    m_channelMarker.setVisible(true); // activate signal on the last setting only

    m_settings.setChannelMarker(&m_channelMarker);
    m_settings.setSpectrumGUI(ui->spectrumGUI);

	m_deviceUISet->registerRxChannelInstance(FreeDVDemod::m_channelIdURI, this);
	m_deviceUISet->addChannelMarker(&m_channelMarker);
	m_deviceUISet->addRollupWidget(this);

	connect(&m_channelMarker, SIGNAL(changedByCursor()), this, SLOT(channelMarkerChangedByCursor()));
    connect(&m_channelMarker, SIGNAL(highlightedByCursor()), this, SLOT(channelMarkerHighlightedByCursor()));
    connect(getInputMessageQueue(), SIGNAL(messageEnqueued()), this, SLOT(handleInputMessages()));

	ui->spectrumGUI->setBuddies(m_spectrumVis->getInputMessageQueue(), m_spectrumVis, ui->glSpectrum);

	m_iconDSBUSB.addPixmap(QPixmap("://dsb.png"), QIcon::Normal, QIcon::On);
    m_iconDSBUSB.addPixmap(QPixmap("://usb.png"), QIcon::Normal, QIcon::Off);
	m_iconDSBLSB.addPixmap(QPixmap("://dsb.png"), QIcon::Normal, QIcon::On);
    m_iconDSBLSB.addPixmap(QPixmap("://lsb.png"), QIcon::Normal, QIcon::Off);

	displaySettings();
	applyBandwidths(5 - ui->spanLog2->value(), true); // does applySettings(true)
}

FreeDVDemodGUI::~FreeDVDemodGUI()
{
    m_deviceUISet->removeRxChannelInstance(this);
	delete m_freeDVDemod; // TODO: check this: when the GUI closes it has to delete the demodulator
	delete m_spectrumVis;
	delete ui;
}

bool FreeDVDemodGUI::blockApplySettings(bool block)
{
    bool ret = !m_doApplySettings;
    m_doApplySettings = !block;
    return ret;
}

void FreeDVDemodGUI::applySettings(bool force)
{
	if (m_doApplySettings)
	{
        FreeDVDemod::MsgConfigureChannelizer* channelConfigMsg = FreeDVDemod::MsgConfigureChannelizer::create(
                m_freeDVDemod->getAudioSampleRate(), m_channelMarker.getCenterFrequency());
        m_freeDVDemod->getInputMessageQueue()->push(channelConfigMsg);

        FreeDVDemod::MsgConfigureFreeDVDemod* message = FreeDVDemod::MsgConfigureFreeDVDemod::create( m_settings, force);
        m_freeDVDemod->getInputMessageQueue()->push(message);
	}
}

void FreeDVDemodGUI::applyBandwidths(int spanLog2, bool force)
{
    bool dsb = ui->dsb->isChecked();
    //int spanLog2 = ui->spanLog2->value();
    m_spectrumRate = m_freeDVDemod->getAudioSampleRate() / (1<<spanLog2);
    int bw = ui->BW->value();
    int lw = ui->lowCut->value();
    int bwMax = m_freeDVDemod->getAudioSampleRate() / (100*(1<<spanLog2));
    int tickInterval = m_spectrumRate / 1200;
    tickInterval = tickInterval == 0 ? 1 : tickInterval;

    qDebug() << "FreeDVDemodGUI::applyBandwidths:"
            << " dsb: " << dsb
            << " spanLog2: " << spanLog2
            << " m_spectrumRate: " << m_spectrumRate
            << " bw: " << bw
            << " lw: " << lw
            << " bwMax: " << bwMax
            << " tickInterval: " << tickInterval;

    ui->BW->setTickInterval(tickInterval);
    ui->lowCut->setTickInterval(tickInterval);

    bw = bw < -bwMax ? -bwMax : bw > bwMax ? bwMax : bw;

    if (bw < 0) {
        lw = lw < bw+1 ? bw+1 : lw < 0 ? lw : 0;
    } else if (bw > 0) {
        lw = lw > bw-1 ? bw-1 : lw < 0 ? 0 : lw;
    } else {
        lw = 0;
    }

    if (dsb)
    {
        bw = bw < 0 ? -bw : bw;
        lw = 0;
    }

    QString spanStr = QString::number(bwMax/10.0, 'f', 1);
    QString bwStr   = QString::number(bw/10.0, 'f', 1);
    QString lwStr   = QString::number(lw/10.0, 'f', 1);

    if (dsb)
    {
        ui->BWText->setText(tr("%1%2k").arg(QChar(0xB1, 0x00)).arg(bwStr));
        ui->spanText->setText(tr("%1%2k").arg(QChar(0xB1, 0x00)).arg(spanStr));
        ui->scaleMinus->setText("0");
        ui->scaleCenter->setText("");
        ui->scalePlus->setText(tr("%1").arg(QChar(0xB1, 0x00)));
        ui->lsbLabel->setText("");
        ui->usbLabel->setText("");
        ui->glSpectrum->setCenterFrequency(0);
        ui->glSpectrum->setSampleRate(2*m_spectrumRate);
        ui->glSpectrum->setSsbSpectrum(false);
        ui->glSpectrum->setLsbDisplay(false);
    }
    else
    {
        ui->BWText->setText(tr("%1k").arg(bwStr));
        ui->spanText->setText(tr("%1k").arg(spanStr));
        ui->scaleMinus->setText("-");
        ui->scaleCenter->setText("0");
        ui->scalePlus->setText("+");
        ui->lsbLabel->setText("LSB");
        ui->usbLabel->setText("USB");
        ui->glSpectrum->setCenterFrequency(m_spectrumRate/2);
        ui->glSpectrum->setSampleRate(m_spectrumRate);
        ui->glSpectrum->setSsbSpectrum(true);
        ui->glSpectrum->setLsbDisplay(bw < 0);
    }

    ui->lowCutText->setText(tr("%1k").arg(lwStr));


    ui->BW->blockSignals(true);
    ui->lowCut->blockSignals(true);

    ui->BW->setMaximum(bwMax);
    ui->BW->setMinimum(dsb ? 0 : -bwMax);
    ui->BW->setValue(bw);

    ui->lowCut->setMaximum(dsb ? 0 :  bwMax);
    ui->lowCut->setMinimum(dsb ? 0 : -bwMax);
    ui->lowCut->setValue(lw);

    ui->lowCut->blockSignals(false);
    ui->BW->blockSignals(false);

    ui->channelPowerMeter->setRange(FreeDVDemodSettings::m_minPowerThresholdDB, 0);

    m_settings.m_dsb = dsb;
    m_settings.m_spanLog2 = spanLog2;
    m_settings.m_rfBandwidth = bw * 100;
    m_settings.m_lowCutoff = lw * 100;

    applySettings(force);

    bool wasBlocked = blockApplySettings(true);
    m_channelMarker.setBandwidth(bw * 200);
    m_channelMarker.setSidebands(dsb ? ChannelMarker::dsb : bw < 0 ? ChannelMarker::lsb : ChannelMarker::usb);
    ui->dsb->setIcon(bw < 0 ? m_iconDSBLSB: m_iconDSBUSB);
    if (!dsb) { m_channelMarker.setLowCutoff(lw * 100); }
    blockApplySettings(wasBlocked);
}

void FreeDVDemodGUI::displaySettings()
{
    m_channelMarker.blockSignals(true);
    m_channelMarker.setCenterFrequency(m_settings.m_inputFrequencyOffset);
    m_channelMarker.setBandwidth(m_settings.m_rfBandwidth * 2);
    m_channelMarker.setTitle(m_settings.m_title);
    m_channelMarker.setLowCutoff(m_settings.m_lowCutoff);

    ui->flipSidebands->setEnabled(!m_settings.m_dsb);

    if (m_settings.m_dsb) {
        m_channelMarker.setSidebands(ChannelMarker::dsb);
    } else {
        if (m_settings.m_rfBandwidth < 0) {
            m_channelMarker.setSidebands(ChannelMarker::lsb);
            ui->dsb->setIcon(m_iconDSBLSB);
        } else {
            m_channelMarker.setSidebands(ChannelMarker::usb);
            ui->dsb->setIcon(m_iconDSBUSB);
        }
    }

    m_channelMarker.blockSignals(false);
    m_channelMarker.setColor(m_settings.m_rgbColor); // activate signal on the last setting only

    setTitleColor(m_settings.m_rgbColor);
    setWindowTitle(m_channelMarker.getTitle());

    blockApplySettings(true);

    ui->deltaFrequency->setValue(m_channelMarker.getCenterFrequency());

    ui->agc->setChecked(m_settings.m_agc);
    ui->agcClamping->setChecked(m_settings.m_agcClamping);
    ui->audioBinaural->setChecked(m_settings.m_audioBinaural);
    ui->audioFlipChannels->setChecked(m_settings.m_audioFlipChannels);
    ui->audioMute->setChecked(m_settings.m_audioMute);
    ui->deltaFrequency->setValue(m_channelMarker.getCenterFrequency());

    // Prevent uncontrolled triggering of applyBandwidths
    ui->spanLog2->blockSignals(true);
    ui->dsb->blockSignals(true);
    ui->BW->blockSignals(true);

    ui->dsb->setChecked(m_settings.m_dsb);
    ui->spanLog2->setValue(5 - m_settings.m_spanLog2);

    ui->BW->setValue(m_settings.m_rfBandwidth / 100.0);
    QString s = QString::number(m_settings.m_rfBandwidth/1000.0, 'f', 1);

    if (m_settings.m_dsb)
    {
        ui->BWText->setText(tr("%1%2k").arg(QChar(0xB1, 0x00)).arg(s));
    }
    else
    {
        ui->BWText->setText(tr("%1k").arg(s));
    }

    ui->spanLog2->blockSignals(false);
    ui->dsb->blockSignals(false);
    ui->BW->blockSignals(false);

    // The only one of the four signals triggering applyBandwidths will trigger it once only with all other values
    // set correctly and therefore validate the settings and apply them to dependent widgets
    ui->lowCut->setValue(m_settings.m_lowCutoff / 100.0);
    ui->lowCutText->setText(tr("%1k").arg(m_settings.m_lowCutoff / 1000.0));

    ui->volume->setValue(m_settings.m_volume * 10.0);
    ui->volumeText->setText(QString("%1").arg(m_settings.m_volume, 0, 'f', 1));

    ui->agcTimeLog2->setValue(m_settings.m_agcTimeLog2);
    s = QString::number((1<<ui->agcTimeLog2->value()), 'f', 0);
    ui->agcTimeText->setText(s);

    ui->agcPowerThreshold->setValue(m_settings.m_agcPowerThreshold);
    displayAGCPowerThreshold(ui->agcPowerThreshold->value());

    ui->agcThresholdGate->setValue(m_settings.m_agcThresholdGate);
    s = QString::number(ui->agcThresholdGate->value(), 'f', 0);
    ui->agcThresholdGateText->setText(s);

    blockApplySettings(false);
}

void FreeDVDemodGUI::displayAGCPowerThreshold(int value)
{
    if (value == FreeDVDemodSettings::m_minPowerThresholdDB)
    {
        ui->agcPowerThresholdText->setText("---");
    }
    else
    {
        QString s = QString::number(value, 'f', 0);
        ui->agcPowerThresholdText->setText(s);
    }
}

void FreeDVDemodGUI::leaveEvent(QEvent*)
{
	m_channelMarker.setHighlighted(false);
}

void FreeDVDemodGUI::enterEvent(QEvent*)
{
	m_channelMarker.setHighlighted(true);
}

void FreeDVDemodGUI::audioSelect()
{
    qDebug("FreeDVDemodGUI::audioSelect");
    AudioSelectDialog audioSelect(DSPEngine::instance()->getAudioDeviceManager(), m_settings.m_audioDeviceName);
    audioSelect.exec();

    if (audioSelect.m_selected)
    {
        m_settings.m_audioDeviceName = audioSelect.m_audioDeviceName;
        applySettings();
    }
}

void FreeDVDemodGUI::tick()
{
    double magsqAvg, magsqPeak;
    int nbMagsqSamples;
    m_freeDVDemod->getMagSqLevels(magsqAvg, magsqPeak, nbMagsqSamples);
    double powDbAvg = CalcDb::dbPower(magsqAvg);
    double powDbPeak = CalcDb::dbPower(magsqPeak);

    ui->channelPowerMeter->levelChanged(
            (FreeDVDemodSettings::m_mminPowerThresholdDBf + powDbAvg) / FreeDVDemodSettings::m_mminPowerThresholdDBf,
            (FreeDVDemodSettings::m_mminPowerThresholdDBf + powDbPeak) / FreeDVDemodSettings::m_mminPowerThresholdDBf,
            nbMagsqSamples);

    if (m_tickCount % 4 == 0) {
        ui->channelPower->setText(tr("%1 dB").arg(powDbAvg, 0, 'f', 1));
    }

    bool squelchOpen = m_freeDVDemod->getAudioActive();

    if (squelchOpen != m_squelchOpen)
    {
        if (squelchOpen) {
            ui->audioMute->setStyleSheet("QToolButton { background-color : green; }");
        } else {
            ui->audioMute->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
        }

        m_squelchOpen = squelchOpen;
    }

    m_tickCount++;
}