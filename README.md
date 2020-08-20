# Linux Block I/O Layer Filter API

## Introduction

Block device filters intercept requests to block devices and perform additional processing. This allows you to create snapshots of block devices, implement change-tracking, and so on...

There are open source kernel modules [block-filter](https://github.com/asimkadav/block-filter), [veeamsnap](https://github.com/veeam/veeamsnap/), [dattobd](https://github.com/datto/dattobd)  and other proprietary ones that implement the functionality of block device filters. The interception was based on the ability to replace the `make_request_fn` function in the request processing queue. 

However, due to the upgrade of the block layer kernel, this trick is no longer possible.

To implement full support for block device filters in the Linux kernel, we suggest creating a special Linux Block Layer Filters API functionality.

## Requirements for Block Device Filters API

* Register and delete a filter "on the fly" from the system without restarting the system or changing the configuration of block devices.
* Provide interception of any requests to block devices.
* Allow multiple filters to work without conflict at the same time. The order in which the request is intercepted is determined by their altitude.
* Allow creating queries without being cycled by filters with a higher altitude, but intercepted by filters with a lower altitude.
* When new block devices appear, send a synchronous request to the registered filter to add it for filtering.
* If a block device is permanently deleted or disappears (surprise removal) in the system, send a synchronous request to remove the device from filtering.
*  

## Why Device Mapper is not the solution

* Device Mapper must be installed in advance and it creates a new device on top of the existing one. In an existing infrastructure, we can't change the configuration of block devices "on the fly".
* You need to intercept requests sent to block devices of the Device Mapper.

## Block Device Filters Algorithm

When uploading to the system, the block device filter is registered in the system. Connection to existing block devices is initiated.

When a new block device the connection of the filters is performed through a callback. This ensures that the filter starts intercepting requests before any requests are initiated to the block device. Device connection requests are called one at a time for all registered filters in the order of their altitude, starting from the lowest levels.

When a request is received for regular removal of block devices from the system, the filters are disabled via callback in the order of their altitude, starting from the higher levels, so that they can normally finish their work with the device.

When a request for unexpected device removal (surprise removal) is received, filters are disabled via callback in the order of their altitude, starting from the lower levels, so that they can crash existing requests from the higher levels.

When the block device filter is unloaded, the block devices are detached. Unregistered.

When a request is intercepted, the request can either be skipped to the lower level or completed. The filter can also create its own requests to the intercepted block device and pass them to a lower level (altitude) for processing.

Creating requests to other block devices is performed by the usual `generic_make_request` or `submit_bio` function . In this case, the request will be intercepted by all the filters in the chain. The filter can separate your own request from others by the context of the request.

All queries from the struct `blk_mq_ops` structure can be filtered, like `make_request_fn`. But maybe can be better place for  hooking? 

Another candidate for interception is a function `submit_bio` or `submit_bio_noacct`. Until recently, this would have been the `generic_make_request` function, but it is no longer available in 5.9. In this case, the interception is performed before it reaches the request processing queue, which means that it does not affect its operation. But there is no certainty that such an interception will be reliable.

## Block Device Filters Functions

Unimplemented yet.
