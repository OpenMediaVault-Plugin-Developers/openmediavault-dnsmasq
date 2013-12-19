<?php
/**
 *
 * @license     http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author      Ian Moore <imooreyahoo@gmail.com>
 * @author      Marcel Beck <marcel.beck@mbeck.org>
 * @author      OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright   Copyright (c) 2011 Ian Moore
 * @copyright   Copyright (c) 2012 Marcel Beck
 * @copyright   Copyright (c) 2013 OpenMediaVault Plugin Developers
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file. If not, see <http://www.gnu.org/licenses/>.
 *
 */
require_once("openmediavault/module.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/util.inc");
require_once("openmediavault/initscript.inc");
require_once("openmediavault/log.inc");

class OMVModuleDnsmasq extends OMVModuleServiceAbstract
    implements OMVINotifyListener, OMVIModuleServiceStatus {

    /**
     * Get the module name.
     */
    public function getName() {
        return "dnsmasq";
    }

    public function applyConfig() {

        $cmd = "omv-mkconf dnsmasq 2>&1";
        if(0 !== $this->exec($cmd, $output)) {
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
        }

        global $xmlConfig;

        $cmd = "omv-mkconf dnsmasq 2>&1";
        if(0 !== $this->exec($cmd, $output)) {
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
        }

        // Restart samba enabling dnsmasq or enabling dns-wins
        // not sure how
        if(FALSE === boolval($this->configGet("//services/dnsmasq/enable")))
            return;

        $initd = new OMVSysVInitScript("dnsmasq", TRUE);
        $initd->restart();
    }

    public function stopService() {
        $initd = new OMVSysVInitScript("dnsmasq", FALSE);
        $initd->exec();
    }

    /**
     * Start the managed service.
     * @throw E_CONFIG_GET_OBJECT_FAILED
     */
    public function startService() {
        global $xmlConfig;
        $xpath = "//services/dnsmasq";
        $object = $xmlConfig->get($xpath);
        if(is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
              $xpath);
        }
        if(FALSE === boolval($object['enable']))
            return;

        $initd = new OMVSysVInitScript("dnsmasq", TRUE);
        $initd->exec();
    }


    /**
     * Update a single entry
     */
    public function updateEntry($conf, $oldconf)
    {
        global $xmlConfig;

        // Create dnsmasq config
        $cmd = "sudo omv-mkconf dnsmasq 2>&1";
        OMVUtil::exec($cmd, $output, $result);
        if ($result !== 0) {
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
        }

        // If WINS support is enabled, we need to
        // regenerate the dnsmasq wins file so that
        // static entries aren't over written by wins
        if ($xmlConfig->get("//services/dnsmasq/dns-wins") == "1") {
            $cmd = "sudo /usr/local/bin/dnsmasq_wins 2>&1";
            OMVUtil::exec($cmd, $output, $result);
            if ($result !== 0) {
                throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
            }
        }

        // Only need to restart if CNAMEs change.
        // Other files are monitored by dnsmasq
        if (empty($conf['cnames'])){
            $conf['cnames'] = '';
        }

        if (empty($oldconf['cnames'])) {
            $oldconf['cnames'] = '';
        }

        if ($conf['cnames'] != $oldconf['cnames']) {
            $initd = new OMVSysVInitScript("dnsmasq");
            $initd->restart();

            // Otherwise, HUP the process
        } else {
            exec("sudo kill -HUP `pidof dnsmasq`");
        }

    }

    /**
     * Get the module status.
     */
    public function getStatus() {
        global $xmlConfig;

        // Get the configuration object
        $xpath  = "//services/dnsmasq";
        $object = $xmlConfig->get($xpath);
        if (is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        $initd = new OMVSysVInitScript("dnsmasq");

        return array(
            "name"    => $this->getName(),
            "title"   => gettext("dnsmasq"),
            "enabled" => ! empty($object['enable']),
            "running" => ($initd->status() == 0)
        );
    }

    /**
     * Bind listeners.
     */
    function bindListeners(OMVNotifyDispatcher $dispatcher)
    {
        /* Update service if configuration has been modified */
        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.dnsmasq",
            array($this, "setDirty"), 1);

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE | OMV_NOTIFY_CREATE,
            "org.openmediavault.services.dnsmasq.entries.entry",
            array($this, "updateEntry"), 1);
    }

}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleDnsmasq());

// Register logging
OMVLogFile::registerType("dnsmasq", array(
    "filename" => "dnsmasq.log",
    "filepath" => "/var/log/dnsmasq.log",
    "regex"    => "/^([^\s]+\s+\d+\s+\d+:\d+:\d+) .*?\[\d+\]: (.*)$/",
    "columns"  => array(
        "date"  => array(
            "index" => 1,
            "func"  => create_function('$v', 'return strpdate($v,"M j G:i:s");')
        ),
        "event" => 2
    )
));

// Register logging
OMVLogFile::registerType("dnsmasq-dhcp", array(
    "filename" => "dnsmasq-dhcp.log",
    "filepath" => "/var/log/dnsmasq-dhcp.log",
    "regex"    => "/^([^\s]+\s+\d+\s+\d+:\d+:\d+) .*?\[\d+\]: (.*)$/",
    "columns"  => array(
        "date"  => array(
            "index" => 1,
            "func"  => create_function('$v', 'return strpdate($v,"M j G:i:s");')
        ),
        "event" => 2
    )
));

?>
