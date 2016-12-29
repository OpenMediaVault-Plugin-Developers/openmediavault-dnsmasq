<?php
/**
 * @license     http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author      Ian Moore <imooreyahoo@gmail.com>
 * @author      Marcel Beck <marcel.beck@mbeck.org>
 * @author      OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright   Copyright (c) 2011 Ian Moore
 * @copyright   Copyright (c) 2012 Marcel Beck
 * @copyright   Copyright (c) 2009-2013 Volker Theile
 * @copyright   Copyright (c) 2013-2016 OpenMediaVault Plugin Developers
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

    /**
     * Update a single entry
     */
    public function updateEntry($conf, $oldconf)
    {
        // Create dnsmasq config
        $cmd = new \OMV\System\Process("omv-mkconf", "dnsmasq");
        $cmd->setRedirect2to1();
        $cmd->execute();

        // If WINS support is enabled, we need to
        // regenerate the dnsmasq wins file so that
        // static entries aren't over written by wins
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.dnsmasq");

        if ($object->get("dns-wins") == "1") {
            $cmd = new \OMV\System\Process("/usr/sbin/dnsmasq_wins");
            $cmd->setRedirect2to1(true);
            $cmd->execute();
            $cmd = " 2>&1";
        }

        // Only need to restart if CNAMEs change.
        // Other files are monitored by dnsmasq
        if ( empty($conf['cnames']) )
            $conf['cnames'] = '';

        if ( empty($oldconf['cnames']))
            $oldconf['cnames'] = '';

        if ($conf['cnames'] != $oldconf['cnames']) {
            $systemCtl = new \OMV\System\SystemCtl("dnsmasq");
            $systemCtl->restart();
        } else {
            // Otherwise, HUP the process
            exec("kill -HUP `pidof dnsmasq`");
        }
    }

    public function bindListeners(\OMV\Engine\Notify\Dispatcher $dispatcher)
    {
        /* Update service if configuration has been modified */
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            "org.openmediavault.conf.service.dnsmasq",
            [ $this, "setDirty" ]
        );
        $dispatcher->addListener(
            OMV_NOTIFY_CREATE | OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE,
            "org.openmediavault.conf.service.dnsmasq.entry",
            [ $this, "updateEntry" ]
        );
    }
}
