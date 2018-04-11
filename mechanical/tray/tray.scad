// Copyright © Michael K Johnson
// May be used under the terms of the MIT license found at
// https://github.com/project-hermes/hermes-sensor/blob/master/LICENSE

d = inches(3); // ID of container
l = 120;
shell = 3; // default thickness of printed parts

board_l = inches(4.06);
board_w = inches(1.42);
hole_x_off = (board_l - inches(3.74)) / 2;
hole_y_off = (board_w - inches(1.107)) / 2;
hole_avg_off = (hole_x_off + hole_y_off) / 2; // close enough
hole_off = hole_avg_off * sqrt(2); // mount at 45⁰ angle

standoff_d = hole_off;
standoff_h = 2; // needs to be larger than the largest excursion below PCB
pcb_z = 1.6; // thickness of PCB
screw_d = inches(0.125); // size of mounting screw hole
clip_d = 0.5; // board retention clip size; best value depends on material
clip_h = 5; // height above the *bottom* of the PCB

e = 0.01; // HACK: make surface non-coincident for quick rendering

function inches(i) = i / 0.039370079; // inches to mm (default units)

module standoff(x, y, z, r) {
    // standoff with screw hole (at origin) and integrated clip
    $fn = 30;
    standoff_r = standoff_d/2;
    translate([x, y, z]) rotate([0, 0, r]) {
        // body below clip
        difference() {
            hull() {
                cylinder(d=standoff_d, h=standoff_h);
                translate([standoff_r, -standoff_r, 0])
                    cube([standoff_r, standoff_d, standoff_h]);
            }
            translate([0, 0, -e])
                cylinder(d=screw_d, h=standoff_h+2*e);
        }
        // clip arm beside pcb
        translate([standoff_r, -standoff_r, standoff_h])
            cube([standoff_r, standoff_d, pcb_z]);
        // clip above pcb
        hull() {
            translate([standoff_r, -standoff_r, standoff_h+pcb_z])
                cube([standoff_r, standoff_d, clip_h-pcb_z]);
            translate([standoff_r, -standoff_r, standoff_h+((clip_h-pcb_z)/2)+pcb_z])
                rotate([-90, 0, 0])
                cylinder(d=clip_d, h=standoff_d);
        }
    }
}
module tray() {
    x_off = (l - board_l) / 2;
    y_off = (d - board_w) / 2;
    centers = [[x_off, y_off, 180+45],
               [x_off, d-y_off, 90+45],
               [l-x_off, y_off, -45],
               [l-x_off, d-y_off, 45]];
    corners = [[-e, e],
               [-e, d-shell+e],
               [l-shell+e, -e],
               [l-shell+e, d-shell+e]];
    difference() {
        union() {
            // base plate
            cube([l, d, shell]);
            // clip standoffs
            for(p=centers) {
                x = p[0];
                y = p[1];
                r = p[2];
                standoff(x, y, shell, r);
            }
        }
        union() {
            // notches to engage rings
            for(p=corners) {
                x = p[0];
                y = p[1];
                translate([x, y, -e])
                    cube([shell+e, shell+e, shell+2*e]);
            }
            // let screws go all the way through the base plate
            for(p=centers) {
                x = p[0];
                y = p[1];
                r = p[2];
                $fn = 30;
                translate([x, y, -e]) cylinder(d=screw_d, h=shell+2*e);
            }
        }
    }
}
module bracelet() {
    $fn = 180;
    difference() {
        cylinder(d=d, h=2*shell);
        union() {
            translate([0, 0, -e])
                cylinder(d=(d-2*shell), h=2*shell+2*e);
            translate([-d/2, -shell/2, shell])
                cube([d, shell, shell+e]);
        }
    }
}
tray();
translate([d/2, 1.5*d+shell, 0]) bracelet();
translate([1.5*d+shell, 1.5*d+shell, 0]) bracelet();
