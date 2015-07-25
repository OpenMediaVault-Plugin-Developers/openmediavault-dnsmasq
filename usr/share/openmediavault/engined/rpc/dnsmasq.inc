<?php
/**
 * @license     http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author      Ian Moore <imooreyahoo@gmail.com>
 * @author      Marcel Beck <marcel.beck@mbeck.org>
 * @author      OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright   Copyright (c) 2011 Ian Moore
 * @copyright   Copyright (c) 2012 Marcel Beck
 * @copyright   Copyright (c) 2009-2013 Volker Theile
 * @copyright   Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
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
 */
require_once("openmediavault/object.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/util.inc");
require_once("openmediavault/rpcservice.inc");
require_once("openmediavault/notify.inc");

class OMVRpcServiceDnsmasq extends OMVRpcServiceAbstract {

    /**
     * Get the RPC service name.
     */
    public function getName() {
        return "Dnsmasq";
    }

    /**
     * Initialize the RPC service.
     */
    public function initialize() {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getLeases");
        $this->registerMethod("getNetworks");
        $this->registerMethod("getEntries");
        $this->registerMethod("getEntry");
        $this->registerMethod("setEntry");
        $this->registerMethod("deleteEntry");
    }

    /**
     * Get the settings configuration object.
     * @param params The method parameters.
     * @param context The context of the caller.
     * @return The requested configuration object.
     * @throw E_CONFIG_GET_OBJECT_FAILED
     */
    public function getSettings($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Get the configuration object.
        $xpath = "//services/dnsmasq";
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Modify the result object.
        $object['enable']          = boolval($object['enable']);
        $object['dhcp-enable']     = boolval($object['dhcp-enable']);
        $object['dns-log-queries'] = boolval($object['dns-log-queries']);
        $object['log-dhcp']        = boolval($object['log-dhcp']);
        $object['dns-wins']        = boolval($object['dns-wins']);

        // Remove useless fields from the result object.
        unset($object['entries']);

        return $object;
    }

    /**
     * Set the settings configuration object.
     * @param params The method parameters.
     * @param context The context of the caller.
     * @return The stored configuration object.
     * @throw E_CONFIG_GET_OBJECT_FAILED
     * @throw E_CONFIG_SET_OBJECT_FAILED
     */
    public function setSettings($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "enable"             : { "type" : "boolean" },
                "dns-log-queries"    : { "type" : "boolean" },
                "dhcp-enable"        : { "type" : "boolean" },
                "dns-wins"           : { "type" : "boolean" },
                "log-dhcp"           : { "type" : "boolean" },
                "domain-name"        : { "type" : "string", "optional" : true},
                "bootfile"           : { "type" : "string", "optional" : true},
                "gateway"            : { "type" : "string"},
                "default-lease-time" : { "type" : "string"},
                "ntp-servers"        : { "type" : "string", "optional" : true},
                "dns-domains"        : { "type" : "string", "optional" : true},
                "wins-servers"       : { "type" : "string", "optional" : true},
                "dns-servers"        : { "type" : "string", "optional" : true},
                "network"            : { "type" : "string", "optional" : true},
                "first-ip"           : { "type" : "string", "optional" : true},
                "last-ip"            : { "type" : "string", "optional" : true},
                "extraoptions"       : { "type" : "string", "optional" : true}
            }
        }');

        // Get the existing configuration object.
        $xpath = "//services/dnsmasq";
        $oldObject = $xmlConfig->get($xpath);
        if(is_null($oldObject))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Update the configuration object.
        $object = array(
            "enable"             => array_boolval($params, 'enable'),
            "dns-log-queries"    => array_boolval($params, 'dns-log-queries'),
            "dhcp-enable"        => array_boolval($params, 'dhcp-enable'),
            "dns-wins"           => array_boolval($params, 'dns-wins'),
            "log-dhcp"           => array_boolval($params, 'log-dhcp'),
            "domain-name"        => $params['domain-name'],
            "bootfile"           => $params['bootfile'],
            "gateway"            => $params['gateway'],
            "default-lease-time" => $params['default-lease-time'],
            "ntp-servers"        => $params['ntp-servers'],
            "dns-domains"        => $params['dns-domains'],
            "wins-servers"       => $params['wins-servers'],
            "dns-servers"        => $params['dns-servers'],
            "network"            => $params['network'],
            "first-ip"           => $params['first-ip'],
            "last-ip"            => $params['last-ip'],
            "extraoptions"       => $params['extraoptions'],
            "entries"            => isset($oldObject['entries']) ? $oldObject['entries'] : array()
        );

        // DHCP isn't enabled if dnsmasq isn't enabled
        $object['dhcp-enable'] = ($object['enable'] && $object['dhcp-enable']);

        if(FALSE === $xmlConfig->replace("//services/dnsmasq", $object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.dnsmasq", $object);

        // Return the configuration object.
        return $object;
    }

    /**
     * Return a list of active leases
     *
     * @param $start     integer start point in paging list
     * @param $count     integer number of objects to return in paged list
     * @param $sortField string field to sort on
     * @param $sortDir   integer sort direction
     *
     * @return array list of leases
     */
     public function getLeases($params, $context)
     {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        $leases = array();
        $exp  = "";
        $name = "";
        $mac  = "";
        $ip   = "";

        $ints = $xmlConfig->getList("//system/network/interfaces/interface");

        // This may not exist if DHCP is not enabled
        if (FALSE === $xmlConfig->get("//services/dnsmasq/dhcp-enable"))
            return $leases;

        if (! file_exists("/var/lib/misc/dnsmasq.leases") )
            return $leases;

        $fp = fopen("/var/lib/misc/dnsmasq.leases", "ro");

        if ($fp === false)
            return $leases;

        while (! feof($fp)) {

            $line = trim(fgets($fp));

            if (! $line)
                continue;

            // Get list of values
            list($exp, $mac, $ip, $name) = preg_split('/\s+/', $line);

            // Set display name
            if ($name != '*') {
                $disp = "{$name} ({$ip})";
            } else {
                $disp = $ip;
                $name = sprintf("%s (%s)", $name, $ip);
            }

            // Calculate expiry
            $exp -= time();

            // No need to calculate time if this is not called from the Leases panel
            if ( $exp <= 0 ) {
                $exp = 'Expired';
            } else {
                $days = floor($exp / 86400);
                $exp -= ($days * 86400);

                $hours = floor($exp / 3600);
                $exp -= ($hours * 3600);

                $mins = floor($exp / 60);

                $exp = array();
                if ($days > 0)
                    $exp[] = "{$days} day(s)";

                if ($hours > 0)
                    $exp[] = "{$hours} hour(s)";

                if ($mins > 0)
                    $exp[] = "{$mins} minute(s)";

                $exp = implode(", ", $exp);
            }

            $leases[] = array(
                'ip'   => $ip,
                'mac'  => $mac,
                'name' => $name,
                'disp' => $disp,
                'exp'  => $exp
            );
        }

        // Filter result
        return $this->applyFilter($leases, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    /**
     * Get a list of static networks for this machine.
     *
     * @return array networks
     */
    public function getNetworks($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        $nets = array();
        $ints = $xmlConfig->getList("//system/network/interfaces/interface");

        foreach ($ints as $i) {
            if (! ($i['netmask'] && $i['address']))
                continue;

            $netid = long2ip(ip2long($i['address']) & ip2long($i['netmask'])) . " / {$i['netmask']}";
            $nets[$netid] = $netid;
        }

        $networks = array();
        foreach ($nets as $n) {
            $networks[] = array('netid'=> $n);
        }

        return $networks;
    }

    /**
     * Get list of Entries
     *
     * @param $start     integer start point in paging list
     * @param $count     integer number of objects to return in paged list
     * @param $sortField string field to sort on
     * @param $sortDir   integer sort direction
     *
     * @return array list of Entries
     */
    public function getEntries($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
            }
        }');

        // Get the configuration object.
        $xpath = "//services/dnsmasq/entries/entry";
        $objects = $xmlConfig->getList($xpath);
        if(is_null($objects))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Filter the result.
        return $this->applyFilter($objects, $params['start'],
            $params['limit'], $params['sortfield'], $params['sortdir']);
    }

    /**
     * Get a single Entry's configuration
     *
     * @param $uuid string configuration uuid of Entry
     *
     * @return array Entry configuration data
     */
    public function getEntry($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Get the configuration object.
        $xpath = sprintf("//services/dnsmasq/entries/entry[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        return $object;
    }

    /**
     * Save Entry configuration data
     *
     * @param $data array Entry configuration
     */
    public function setEntry($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"        : "object",
            "properties"  : {
                "uuid"   : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "ip"     : { "type" : "string", "optional" : true },
                "name"   : { "type" : "string", "optional" : true },
                "cnames" : { "type" : "string", "optional" : true },
                "mac"    : { "type" : "string", "optional" : true }
            }
        }');

        // Prepare configuration data.
        $object = array(
            "uuid"   => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "ip"     => $params['ip'],
            "name"   => strtolower($params['name']),
            "cnames" => strtolower($params['cnames']),
            "mac"    => strtolower($params['mac'])
        );

        // Set the configuration data.
        $success = FALSE;

        // Check uniqueness
        if (! empty($object['name']) && TRUE === $xmlConfig->exists("//services/dnsmasq/entries/entry[name='{$object['name']}']")) {

            $ex = $xmlConfig->get("//services/dnsmasq/entries/entry[name='{$object['name']}']");

            if ($ex['uuid'] != $object['uuid'] && $ex['ip'] == $data['ip']) {
                throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_UNIQUENESS);
            }
        }

        // Check uniqueness
        if (! empty($data['mac']) && TRUE === $xmlConfig->exists("//services/dnsmasq/entries/entry[ip='{$object['ip']}']")) {

            $ex = $xmlConfig->get("//services/dnsmasq/entries/entry[ip='{$object['ip']}']");

            if ($ex['uuid'] != $object['uuid'] && $ex['mac'] == $data['mac']) {
                throw new OMVException(OMVErrorMsg::E_CONFIG_OBJECT_UNIQUENESS);
            }
        }

        // CNAMES must be unique
        if (! empty($object['cnames'])) {
            $cnames = str_replace(array(" ", "'", '"', '/', '\\'), '', trim($object['cnames']));
            $cnames = preg_replace('/\s*,\s*/', ', ', $cnames);
            $cnames = array_unique(explode(', ', $cnames));

            foreach ($cnames as $cname) {
                // check for exact name
                try {
                    $ex = $xmlConfig->get("//services/dnsmasq/entries/entry[name='{$cname}']");
                } catch (Exception $e) {
                    // ignore
                    $ex = null;
                }

                if ($ex && $ex['uuid'] != $object['uuid'])
                    throw new Exception("An entry with the host name (or other names) '{$cname}' already exists.");

                $exnames = $xmlConfig->getList("//services/dnsmasq/entries/entry[contains(cnames, '{$cname}')]");

                foreach ($exnames as $ex) {
                    // Skip ourselves
                    if ($object['uuid'] == $ex['uuid'])
                        continue;

                    if (preg_match("/(^|\s){$cname}(,|$)/", $ex['cnames']))
                        throw new Exception("An entry with the host name (or other names) '{$cname}' already exists.");
                }
            }

            $object['cnames'] = implode(', ', $cnames);
        }

        // Set configuration data
        if ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) {
            // Append the configuration object.
            $success = $xmlConfig->set("//services/dnsmasq/entries", array("entry" => $object));
        } else {
            // Update the existing configuration object.
            $success = $xmlConfig->replace("//services/dnsmasq/entries/entry[uuid='{$params['uuid']}']", $object);
        }

        if(FALSE === $success)
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ?
            OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.dnsmasq.entries.entry", $object);

        // Return the configuration object.
        return $object;
    }

    /**
     * Remove Entry from config
     *
     * @param string $uuid configuration uuid of Entry
     */
    public function deleteEntry($params, $context) {

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xpath = sprintf("//services/dnsmasq/entries/entry[uuid='%s']", $params['uuid']);
        $object = $this->deleteConfigObjectByPath($xpath,
            "org.openmediavault.services.dnsmasq.entries.entry");

        // Return the deleted configuration object.
        return $object;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceDnsmasq());
