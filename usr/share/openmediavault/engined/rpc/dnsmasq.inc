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
 */

class OMVRpcServiceDnsmasq extends \OMV\Rpc\ServiceAbstract
{
    public function getName()
    {
        return 'Dnsmasq';
    }

    public function initialize()
    {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
        $this->registerMethod('getLeases');
        $this->registerMethod('getNetworks');
        $this->registerMethod('getEntryList');
        $this->registerMethod('getEntry');
        $this->registerMethod('setEntry');
        $this->registerMethod('deleteEntry');
    }

    public function getSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.dnsmasq');
        // Remove useless properties from the object.
        $object->remove('entries');
        return $object->getAssoc();
    }

    public function setSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.dnsmasq.setsettings');
        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.dnsmasq');
        $object->setAssoc($params);
        // DHCP isn't enabled if dnsmasq isn't enabled
        $object->set('dhcp-enable', $params['enable'] && $params['dhcp-enable']);
        $db->set($object);
        // Remove useless properties from the object.
        $object->remove('entries');
        // Return the configuration object.
        return $object->getAssoc();
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
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $leases = [];
        $exp  = '';
        $name = '';
        $mac  = '';
        $ip   = '';

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.dnsmasq');

        // This may not exist if DHCP is not enabled
        if (false === $object->get('dhcp-enable')) {
            return $leases;
        }
        if ( ! file_exists('/var/lib/misc/dnsmasq.leases') ) {
            return $leases;
        }
        $fp = fopen('/var/lib/misc/dnsmasq.leases', 'ro');

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
                $disp = sprintf('%s (%s)', $name, $ip);
            } else {
                $disp = $ip;
                $name = sprintf('%s (%s)', $name, $ip);
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

                $exp = [];
                if ($days > 0)
                    $exp[] = sprintf('%s days(s)', $days);

                if ($hours > 0)
                    $exp[] = sprintf('%s hour(s)', $hours);

                if ($mins > 0)
                    $exp[] = sprintf('%s minutes(s)', $mins);

                $exp = implode(', ', $exp);
            }

            $leases[] = [
                'ip'   => $ip,
                'mac'  => $mac,
                'name' => $name,
                'disp' => $disp,
                'exp'  => $exp
            ];
        }

        // Filter result
        return $this->applyFilter($leases, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    /**
     * Get a list of static networks for this machine.
     */
    public function getNetworks($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->get('conf.system.network.interface');

        $nets = [];
        foreach ($objects as $objectk => &$objectv) {
            if (! ($objectv->get('netmask') && $objectv->get('address')))
                continue;

            $netid = long2ip(ip2long($objectv->get('address')) & ip2long($objectv->get('netmask'))) . ' / ' . $objectv->get('netmask');
            $nets[$netid] = $netid;
        }
        $networks = [];
        foreach ($nets as $n) {
            $networks[] = [ 'netid' => $n ];
        }
        // Filter result
        return $networks;
    }

    public function getEntryList($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.getlist');
        // Get the list of configuration objects.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->getAssoc('conf.service.dnsmasq.entry');
        // Filter the result objects.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getEntry($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        return $db->getAssoc('conf.service.dnsmasq.entry', $params['uuid']);
    }

    public function setEntry($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.dnsmasq.setentry');
        // Prepare the configuration object.
        $object = new \OMV\Config\ConfigObject('conf.service.dnsmasq.entry');
        $object->setAssoc($params);
        // Set the configuration object.
        $isNew = $object->isNew();
        $db = \OMV\Config\Database::getInstance();
        if (TRUE === $isNew) {
            // Check uniqueness - cnames, ip, name
            if (!empty($params['cnames'])) {
                $db->assertIsUnique($object, 'cnames');
            }
            $db->assertIsUnique($object, 'ip');
            if (!empty($params['name'])) {
                $db->assertIsUnique($object, 'name');
            }
        }
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function deleteEntry($params, $context)
    {
       // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Delete the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.dnsmasq.entry', $params['uuid']);
        $db->delete($object);
        // Return the deleted configuration object.
        return $object->getAssoc();
    }
}
