// Copyright © Michael K Johnson
// May be used under the terms of the MIT license found at
// https://github.com/project-hermes/hermes-sensor/blob/master/LICENSE

slop = 0.5; // loose interference fit
sl = slop/2; // for distributing slop over two parts
d = inches(3); // ID of container
cd = 62; // ID of caps
l = 120; // overall support length, must be less than 124
al = 90; // length of acrylic section
bl = 25; // inset into bottom cap
tl = al+bl; // offset for top cap (l-tl <= 7)
shell = 3; // default thickness of printed parts

// the case acrylic is 132mm long
// the caps extend 21mm into the acrylic
// this leaves 132-21*2 = 90mm max at diameter d
// inside the caps there is 27mm at diameter cd
// the bottom cap has no protrusions
// the top cap has 7mm of usable space below hardware
// this gives 90+7+27 = 124 max length

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
    corners = [[bl-e, -e],
               [bl-e, d-shell+e],
               [tl-shell+e, -e],
               [tl-shell+e, d-shell+e]];
    intersection() {
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
        // the whole thing is constrained by the enclosing cylinders
        union() {
            translate([bl, d/2, shell/2]) rotate([0, 90, 0])
                cylinder(d=d, h=al);
            translate([0, d/2, shell/2]) rotate([0, 90, 0])
                cylinder(d=cd, h=bl);
            translate([tl, d/2, shell/2]) rotate([0, 90, 0])
                cylinder(d=cd, h=bl);
        }
    }
}
module bracelet() {
    $fn = 180;
    difference() {
        cylinder(d=d-slop, h=2*shell+slop);
        union() {
            translate([0, 0, -e])
                cylinder(d=(d-slop-2*shell), h=2*shell+2*e+slop);
            translate([-d/2, -(shell/2 + sl), shell-slop])
                cube([d, shell+slop, shell+e+2*slop]);
        }
    }
}
tray();
translate([d/2, 1.5*d+shell, 0]) bracelet();
translate([1.5*d+shell, 1.5*d+shell, 0]) bracelet();
