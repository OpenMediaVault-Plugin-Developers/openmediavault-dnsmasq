/**
 * @license     http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author      Ian Moore <imooreyahoo@gmail.com>
 * @author      Marcel Beck <marcel.beck@mbeck.org>
 * @author      OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright   Copyright (c) 2011 Ian Moore
 * @copyright   Copyright (c) 2012 Marcel Beck
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
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/form/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")

Ext.define("OMV.module.admin.service.dnsmasq.Settings", {
    extend : "OMV.workspace.form.Panel",

    plugins : [{
        ptype        : "linkedfields",
        correlations : [{
            name       : [
                "gateway"
            ],
            conditions : [{
                name  : "dhcp-enable",
                value : false
            }],
            properties : "allowBlank"
        }]
    }],

    initComponent : function () {
        var me = this;

        me.on('load', function () {
            var checked = me.findField('enable').checked;
            var dchecked = me.findField('dhcp-enable').checked;
            var parent = me.up('tabpanel');

            if (!parent)
                return;

            var staticPanel = parent.down('panel[title=' + _("Static Entries") + ']');
            var leasesPanel = parent.down('panel[title=' + _("Leases") + ']');

            if (staticPanel) {
                checked ? staticPanel.enable() : staticPanel.disable();
            }
            if (leasesPanel) {
                dchecked ? leasesPanel.enable() : leasesPanel.disable();
            }
        });

        me.callParent(arguments);
    },

    rpcService   : "Dnsmasq",
    rpcGetMethod : "getSettings",
    rpcSetMethod : "setSettings",

    getFormItems: function () {
        return [{
            xtype    : "fieldset",
            title    : _("General"),
            defaults : {
                labelSeparator : ""
            },
            items    : [{
                xtype      : "checkbox",
                name       : "enable",
                fieldLabel : _("Enable"),
                checked    : false
            },{
                xtype      : "textfield",
                name       : "domain-name",
                fieldLabel : _("Domain Name"),
                allowBlank : true,
                value      : "local",
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Configures local DNS entries to contain the domain name above. Also sets the domain for DHCP clients.")
                }]
            }]
        },{
            xtype    : "fieldset",
            title    : _("DNS Settings"),
            defaults : {
                labelSeparator : ""
            },
            items    : [{
                border : false,
                html   : "<p>" +
                         _("The local DNS server will respond to DNS queries for the hosts specified on the Static Entries tab, (optionally) hosts learned through OpenMediaVault's WINS server, and (optionally) DHCP clients that send their host name in DHCP requests. DNS requests for unknown hosts are forwarded to the OpenMediaVault's DNS servers as configured in System -> Network -> DNS Server." ) +
                         "</p>"
            },{
                xtype      : "checkbox",
                name       : "dns-log-queries",
                fieldLabel : _("Log Queries"),
                boxLabel   : _("For debugging purposes, log each DNS query"),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "dns-wins",
                fieldLabel : _("Use WINS entries"),
                boxLabel   : _("Use IP / name entries obtained through WINS server."),
                checked    : false,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Requires that Enable WINS server is set in Services -> SMB/CIFS")
                }]
            }]
        },{
            xtype    : "fieldset",
            title    : _("DHCP Settings"),
            defaults : {
                labelSeparator : ""
            },
            items    : [{
                xtype      : "checkbox",
                name       : "dhcp-enable",
                fieldLabel : _("Enable"),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "log-dhcp",
                fieldLabel : _("Log DHCP"),
                boxLabel   : _("Log lots of extra information about DHCP transactions."),
                checked    : false
            },{
                xtype         : "combo",
                name          : "network",
                hiddenName    : "network",
                fieldLabel    : _("Lease Network"),
                emptyText     : _("Select a network ..."),
                allowBlank    : true,
                editable      : false,
                triggerAction : "all",
                displayField  : "netid",
                valueField    : "netid",
                store         : Ext.create("OMV.data.Store", {
                    autoLoad : true,
                    model    : OMV.data.Model.createImplicit({
                        idProperty  : "netid",
                        fields      : [
                            { name : "netid", type : "string" }
                        ]
                    }),
                    proxy : {
                        type : "rpc",
                        rpcData : {
                            service : "Dnsmasq",
                            method  : "getNetworks"
                        },
                        appendSortParams : false
                    },
                    sorters : [{
                        direction : "ASC",
                        property  : "netid"
                    }]
                })
            },{
                xtype      : "textfield",
                name       : "gateway",
                fieldLabel : _("Gateway"),
                vtype      : "IPv4",
                allowBlank : true,
                value      : ""
            },{
                xtype      : "textfield",
                name       : "first-ip",
                fieldLabel : _("First IP address"),
                vtype      : "IPv4",
                allowBlank : true,
                value      : ""
            },{
                xtype      : "textfield",
                name       : "last-ip",
                vtype      : "IPv4",
                fieldLabel : _("Last IP address"),
                allowBlank : true,
                value      : ""
            },{
                xtype         : "combo",
                name          : "default-lease-time",
                fieldLabel    : _("Lease Time"),
                allowBlank    : false,
                displayField  : "text",
                valueField    : "value",
                value         : "24h",
                triggerAction : "all",
                mode          : "local",
                store         : new Ext.data.SimpleStore({
                    fields : [ "value", "text" ],
                    data   : [
                        [ "1h", _("1 hour") ],
                        [ "3h", _("3 hours") ],
                        [ "6h", _("6 hours") ],
                        [ "12h", _("12 hours") ],
                        [ "24h", _("1 day") ],
                        [ "48h", _("2 days") ],
                        [ "96h", _("4 days") ],
                        [ "168h", _("1 week") ]
                    ]
                })
            },{
                xtype      : "textfield",
                name       : "dns-domains",
                fieldLabel : _("DNS Search Domain(s)"),
                allowBlank : true,
                value      : "",
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Separate multiple entries with commas.")
                }]
            },{
                xtype      : "textfield",
                name       : "wins-servers",
                fieldLabel : _("WINS Server(s)"),
                allowBlank : true,
                value      : "",
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Separate multiple entries with commas.")
                }]
            },{
                xtype      : "textfield",
                name       : "ntp-servers",
                fieldLabel : _("NTP Server(s)"),
                allowBlank : true,
                value      : "",
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Separate multiple entries with commas.")
                }]
            },{
                xtype      : "textfield",
                name       : "dns-servers",
                fieldLabel : _("DNS Server(s)"),
                allowBlank : true,
                value      : "",
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Separate multiple entries with commas.") + " " +
                            _("If you only want to use this DNS Server, just enter the IP of this host.")
                }]
            },{
                xtype      : "textfield",
                name       : "bootfile",
                fieldLabel : _("DHCP Boot"),
                allowBlank : true,
                value      : "",
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("If set, this file must exist on the TFTP share. Example: /pxelinux.0,0.0.0.0")
                }]
            }]
        },{
            xtype    : "fieldset",
            title    : _("Extra"),
            defaults : {
                labelSeparator : ""
            },
            items    : [{
                xtype      : "textarea",
                name       : "extraoptions",
                fieldLabel : _("Extra options"),
                allowBlank : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Extra options for dnsmasq configuration file.")
                }]
            }]
        }];
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "settings",
    path      : "/service/dnsmasq",
    text      : _("Settings"),
    position  : 10,
    className : "OMV.module.admin.service.dnsmasq.Settings"
});
