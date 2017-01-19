<?php
/**
 * @license     http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author      Ian Moore <imooreyahoo@gmail.com>
 * @author      Marcel Beck <marcel.beck@mbeck.org>
 * @author      OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright   Copyright (c) 2011 Ian Moore
 * @copyright   Copyright (c) 2012 Marcel Beck
 * @copyright   Copyright (c) 2009-2013 Volker Theile
 * @copyright   Copyright (c) 2013-2017 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

class OMVModuleDnsmasq extends \OMV\Engine\Module\ServiceAbstract
    implements \OMV\Engine\Notify\IListener, \OMV\Engine\Module\IServiceStatus
{
    public function getName()
    {
        return "dnsmasq";
    }

    public function applyConfig()
    {
        $cmd = new \OMV\System\Process("omv-mkconf", "dnsmasq");
        $cmd->setRedirect2to1();
        $cmd->execute();
    }

    public function stopService()
    {
        $systemCtl = new \OMV\System\SystemCtl("dnsmasq");
        $systemCtl->disable(TRUE);
    }

    public function startService()
    {
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.dnsmasq");
        if (TRUE !== $object->get("enable"))
            return;
        // Start this service and enable the unit file.
        $systemCtl = new \OMV\System\SystemCtl("dnsmasq");
        $systemCtl->enable(TRUE);
    }

    public function getStatus()
    {
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.dnsmasq");
        $systemCtl = new \OMV\System\SystemCtl("dnsmasq");
        return [
            "name" => $this->getName(),
            "title" => gettext("DNSMasq"),
            "enabled" => $object->get("enable"),
            "running" => $systemCtl->isActive()
        ];
    }

    public function bindListeners(\OMV\Engine\Notify\Dispatcher $dispatcher)
    {
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            "org.openmediavault.conf.service.dnsmasq",
            [ $this, "setDirty" ]
        );
        $dispatcher->addListener(
            OMV_NOTIFY_CREATE | OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE,
            "org.openmediavault.conf.service.dnsmasq.entry",
            [ $this, "setDirty" ]
        );
    }
}
