// Copyright © Michael K Johnson
// May be used under the terms of the MIT license found at
// https://github.com/project-hermes/hermes-sensor/blob/master/LICENSE

slop = 0.35; // tight interference fit
sl = slop/2; // for distributing slop over two parts
d = inches(3); // ID of container
cd = 62; // ID of caps
l = 120; // overall support length, must be less than 124
al = 90; // length of acrylic section
bl = 25; // inset into bottom cap
tl = al+bl; // offset for top cap (l-tl <= 7)
shell = 2; // default thickness of printed parts
rh = shell*3; // height of supporting ring
rshell = 4; // thickness of supporting ring

// the case acrylic is 132mm long
// the caps extend 21mm into the acrylic
// this leaves 132-21*2 = 90mm max at diameter d (al)
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
board_x_off = (l - board_l) / 2;
board_y_off = (d - board_w) / 2;
standoff_d = hole_off;
standoff_r = standoff_d/2;
standoff_h = 15; // needs to be larger than the largest excursion below PCB

board_centers = [ // board standoff center locations and orientation
    // X, Y, rotation of standoff
    [board_x_off+hole_x_off, board_y_off+hole_y_off, 180+45],
    [board_x_off+hole_x_off, d-(board_y_off+hole_y_off), 90+45],
    [l-(board_x_off+hole_x_off), board_y_off+hole_y_off, -45],
    [l-(board_x_off+hole_x_off), d-(board_y_off+hole_y_off), 45]
];
ant_l = 80; // antenna length
ant_w = 20; // antenna width
ant_n_x_off = 10; // antenna needs offset
ant_centers = [// antenna standoff center locations / orientation
    [l-(ant_n_x_off+standoff_r), d/2, 0],
    [l-(ant_n_x_off+ant_l-standoff_r), d/2, 180],
    [l-(ant_n_x_off+ant_l/3), (d/2-ant_w/2)+standoff_r, -90],
    [l-(ant_n_x_off+ant_l/3), d/2+ant_w/2-standoff_r, 90]
];
bat_z = 10;
bat_l = 35;
bat_w = 50;
bat_d = 4; // size of hole for battery zip-tie
cord_d = 8; // holes to route cords through
cord_centers = [
    [bl, 1/4*d],
    [bl, 3/4*d],
    [l-bl, 1/4*d],
    [l-bl, 3/4*d],
];
pcb_z = 1.6; // thickness of PCB
screw_d = inches(0.125); // size of mounting screw hole
clip_d = 0.5; // board retention clip size; best value depends on material
clip_h = 5; // height above the *bottom* of the PCB

e = 0.01; // HACK: make surface non-coincident for quick rendering

function inches(i) = i / 0.039370079; // inches to mm (default units)

module standoff(x, y, z, r, hole=true) {
    // standoff with optional screw hole (at origin) and integrated clip
    $fn = 30;
    translate([x, y, z]) rotate([0, 0, r]) {
        // body below clip
        difference() {
            hull() {
                cylinder(d=standoff_d, h=standoff_h);
                translate([standoff_r, -standoff_r, 0])
                    cube([standoff_r, standoff_d, standoff_h]);
            }
            if (hole) {
                translate([0, 0, -e])
                    cylinder(d=screw_d, h=standoff_h+2*e);
            }
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
    corners = [[bl-e, -e],
               [bl-e, e+d-(rshell+slop+sl)],
               [tl-(shell+slop), -e],
               [tl-(shell+slop), e+d-(rshell+slop+sl)]];
    intersection() {
        difference() {
            union() {
                // base plate
                cube([l, d, shell]);
                // antena clip standoffs
                for(p=ant_centers) {
                    x = p[0];
                    y = p[1];
                    r = p[2];
                    standoff(x, y, shell, r, false);
                }
            }
            union() {
                // notches to engage rings
                for(p=corners) {
                    x = p[0];
                    y = p[1];
                    translate([x, y, -e])
                        cube([shell+e+slop, rshell+e+slop, shell+2*e]);
                }
                // let screws go all the way through the base plate
                for(p=board_centers) {
                    x = p[0];
                    y = p[1];
                    $fn = 30;
                    translate([x, y, -e]) cylinder(d=screw_d, h=shell+2*e);
                }
                x=bat_l/2;
                y=bat_w/2;
                for(p=[[0, -y], [0, y], [-x, 0], [x, 0]]) {
                    translate([(l/2+p[0])-(bat_d/2), (d/2+p[1])-(bat_d/2), -e])
                        cube([bat_d, bat_d, shell+2*e]);
                }
                for(p=cord_centers) {
                    x = p[0];
                    y = p[1];
                    $fn = 30;
                    translate([x, y, -e]) cylinder(d=cord_d, h=shell+2*e);
                }
            }
        }
        // the whole thing is constrained by the enclosing cylinders
        union() {
            translate([bl, d/2, shell/2]) rotate([0, 90, 0])
                cylinder(d=d-slop, h=al, $fn=360);
            translate([0, d/2, shell/2]) rotate([0, 90, 0])
                cylinder(d=cd, h=bl, $fn=360);
            translate([tl, d/2, shell/2]) rotate([0, 90, 0])
                cylinder(d=cd, h=bl, $fn=360);
        }
    }
}
module bracelet() {
    $fn = 180;
    difference() {
        cylinder(d=d-slop, h=rh);
        union() {
            translate([0, 0, -e])
                cylinder(d=(d-slop-2*rshell), h=rh+2*e);
            translate([-d/2, -(shell/2 + sl), shell])
                cube([d, shell+slop, rh-shell+e]);
        }
    }
}
module test_fit() {
    %translate([bl, (d-slop)/2, shell/2]) rotate([90, 0, 90])
        bracelet();
    %translate([tl, (d-slop)/2, shell/2]) rotate([90, 0, -90])
        bracelet();
    %translate([l/2-board_l/2, d/2-board_w/2, -shell])
        cube([board_l, board_w, pcb_z]);
    %translate([l-(ant_l+ant_n_x_off), d/2-ant_w/2, standoff_h+shell])
        cube([ant_l, ant_w, pcb_z]);
    %translate([l/2-bat_l/2, d/2-bat_w/2, shell])
        cube([bat_l, bat_w, bat_z]);
}
tray();
translate([d/2, 1.5*d+shell, 0]) bracelet();
translate([1.5*d+shell, 1.5*d+shell, 0]) bracelet();
//test_fit();
