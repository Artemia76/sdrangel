///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2020 Edouard Griffiths, F4EXB                                   //
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


#include <QtPlugin>
#include "plugin/pluginapi.h"

#ifndef SERVER_MODE
#include "filesinkgui.h"
#endif
#include "filesink.h"
#include "filesinkwebapiadapter.h"
#include "filesinkplugin.h"

const PluginDescriptor FileSinkPlugin::m_pluginDescriptor = {
    FileSink::m_channelId,
    QString("File Sink"),
    QString("4.15.0"),
    QString("(c) Edouard Griffiths, F4EXB"),
    QString("https://github.com/f4exb/sdrangel"),
    true,
    QString("https://github.com/f4exb/sdrangel")
};

FileSinkPlugin::FileSinkPlugin(QObject* parent) :
    QObject(parent),
    m_pluginAPI(0)
{
}

const PluginDescriptor& FileSinkPlugin::getPluginDescriptor() const
{
    return m_pluginDescriptor;
}

void FileSinkPlugin::initPlugin(PluginAPI* pluginAPI)
{
    m_pluginAPI = pluginAPI;

    // register channel Source
    m_pluginAPI->registerRxChannel(FileSink::m_channelIdURI, FileSink::m_channelId, this);
}

#ifdef SERVER_MODE
PluginInstanceGUI* FileSinkPlugin::createRxChannelGUI(
        DeviceUISet *deviceUISet,
        BasebandSampleSink *rxChannel) const
{
    return nullptr;
}
#else
PluginInstanceGUI* FileSinkPlugin::createRxChannelGUI(DeviceUISet *deviceUISet, BasebandSampleSink *rxChannel) const
{
    return FileSinkGUI::create(m_pluginAPI, deviceUISet, rxChannel);
}
#endif

BasebandSampleSink* FileSinkPlugin::createRxChannelBS(DeviceAPI *deviceAPI) const
{
    return new FileSink(deviceAPI);
}

ChannelAPI* FileSinkPlugin::createRxChannelCS(DeviceAPI *deviceAPI) const
{
    return new FileSink(deviceAPI);
}

ChannelWebAPIAdapter* FileSinkPlugin::createChannelWebAPIAdapter() const
{
	return new FileSinkWebAPIAdapter();
}
