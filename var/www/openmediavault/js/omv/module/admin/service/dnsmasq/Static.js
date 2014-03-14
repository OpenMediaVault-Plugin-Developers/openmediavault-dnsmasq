/**
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
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")

Ext.define("OMV.module.admin.service.dnsmasq.Entries", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc"
    ],
    uses     : [
        "OMV.module.admin.service.dnsmasq.Entry"
    ],

    hidePagingToolbar : false,
    id                : 'DNSMasqEntriesGridPanel',
    stateful          : true,
    stateId           : "9889057b-b2c0-4c48-a4c1-8c9b4fb54d7b",
    columns           : [{
        text      : _("Host Name"),
        sortable  : true,
        dataIndex : "name"
    },{
        text      : _("IP Address"),
        sortable  : true,
        dataIndex : "ip"
    }, {
        text      : _("Other Names"),
        sortable  : true,
        dataIndex : "cnames"
    }, {
        text      : _("MAC Address"),
        sortable  : true,
        dataIndex : "mac"
    }],

    initComponent: function () {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty  : "uuid",
                    fields      : [
                        { name : "uuid", type: "string" },
                        { name : "name", type: "string" },
                        { name : "cnames", type: "string" },
                        { name : "ip", type: "string" },
                        { name : "mac", type: "string" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Dnsmasq",
                        method  : "getEntries"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    onAddButton : function () {
        var me = this;
        Ext.create("OMV.module.admin.service.dnsmasq.Entry", {
            title     : _("Add static entry"),
            uuid      : OMV.UUID_UNDEFINED,
            listeners : {
                scope  : me,
                submit : function () {
                    this.doReload();
                }
            }
        }).show();
    },

    onEditButton : function () {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.module.admin.service.dnsmasq.Entry", {
            title     : _("Edit static entry"),
            uuid      : record.get("uuid"),
            listeners : {
                scope  : me,
                submit : function () {
                    this.doReload();
                }
            }
        }).show();
    },

    doDeletion : function (record) {
        var me = this;
        OMV.Rpc.request({
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "Dnsmasq",
                method  : "deleteEntry",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    }
});

Ext.define("OMV.module.admin.service.dnsmasq.Entry", {
    extend : "OMV.workspace.window.Form",
    uses   : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    rpcService   : "Dnsmasq",
    rpcGetMethod : "getEntry",
    rpcSetMethod : "setEntry",
    plugins      : [{
        ptype : "configobject"
    }],

    width        : 600,

    getFormItems : function () {
        return [{
            border : false,
            html   : _("To create a DNS entry, specify IP address and Host Name. Optionally, you may enter other names which  the host should be known as.<br /><br />")
        },{
            border : false,
            html   : _("Specifying a MAC Address and IP Address will create a static IP DHCP entry.<br /><br />Entering all fields will create an all-in-one static ip reservation and DNS entry.<br /><br />")
        },{
            xtype      : "textfield",
            name       : "name",
            itemId     : "name",
            fieldLabel : _("Host Name"),
            allowBlank : true,
            plugins    : [{
                ptype : "fieldinfo",
                text  : _("If this field is left blank, the host name will be obtained from the client's DHCP request. Beware that not all clients send their host name.")
            }]
        },{
            xtype      : "textfield",
            name       : "cnames",
            fieldLabel : _("Other Names"),
            allowBlank : true,
            plugins    : [{
                ptype : "fieldinfo",
                text  : _("Other host names that should resolve to the specified IP address. Separate multiple entries with commas.")
            }]
        },{
            xtype      : "textfield",
            name       : "ip",
            vtype      : "IPv4",
            fieldLabel : _("IP Address"),
            allowBlank : false
        },{
            xtype      : "textfield",
            name       : "mac",
            fieldLabel : _("MAC Address"),
            allowBlank : true
        },{
            xtype         : "combo",
            name          : "exlease",
            submitValue   : false,
            fieldLabel    : "Lease",
            emptyText     : _("Select existing lease ..."),
            allowBlank    : true,
            allowNone     : true,
            editable      : false,
            triggerAction : "all",
            displayField  : "disp",
            valueField    : "mac",
            listeners     : {
                select : function (a, b, c) {
                    this.ownerCt.getComponent('ip').setValue(b.data.ip);
                    this.ownerCt.getComponent('mac').setValue(b.data.mac);
                    this.ownerCt.getComponent('name').setValue(b.data.name);
                }
            },
            store         : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty  : "mac",
                    fields      : [
                        { name : "ip", type : "string" },
                        { name : "mac", type : "string" },
                        { name : "name", type : "string" },
                        { name : "disp", type : "string" }
                    ]
                }),
                proxy : {
                    type    : "rpc",
                    rpcData : {
                        service : "Dnsmasq",
                        method  : "getLeases"
                    },
                    appendSortParams : false
                },
                sorters : [{
                    direction : "ASC",
                    property  : "mac"
                }]
            })
        }];
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "static",
    path      : "/service/dnsmasq",
    text      : _("Static Entries"),
    position  : 20,
    className : "OMV.module.admin.service.dnsmasq.Entries"
});
