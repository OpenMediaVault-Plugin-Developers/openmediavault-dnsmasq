<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @copyright Copyright (c) 2013-2017 OpenMediaVault Plugin Developers
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
 require_once("openmediavault/functions.inc");

\OMV\System\LogFileSpec::registerSpecification("dnsmasq", [
    "filename" => "dnsmasq.log",
    "filepath" => "/var/log/dnsmasq.log",
    "regex"    => "/^([^\s]+\s+\d+\s+\d+:\d+:\d+) .*?\[\d+\]: (.*)$/",
    "columns"  => [
        "date"  => [
            "index" => 1,
            "func"  => create_function('$v', 'return strpdate($v,"M j G:i:s");')
        ],
        "event" => 2
    ]
]);

// Register logging
\OMV\System\LogFileSpec::registerSpecification("dnsmasq-dhcp", [
    "filename" => "dnsmasq-dhcp.log",
    "filepath" => "/var/log/dnsmasq-dhcp.log",
    "regex"    => "/^([^\s]+\s+\d+\s+\d+:\d+:\d+) .*?\[\d+\]: (.*)$/",
    "columns"  => [
        "date"  => [
            "index" => 1,
            "func"  => create_function('$v', 'return strpdate($v,"M j G:i:s");')
        ],
        "event" => 2
    ]
]);
