#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        //TODO
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        //TODO
        return  glm::lookAt(this->cameraPosition, this->cameraTarget, this->cameraUpDirection);
        //  return glm::mat4();
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        glm::vec3 cameraFront = glm::normalize(cameraTarget - cameraPosition);
        glm::vec3 cameraSide = glm::normalize(glm::cross(cameraFront, cameraUpDirection));

        if (direction == MOVE_FORWARD)
        {
            cameraPosition += cameraFront * speed;
            cameraTarget += cameraFront * speed;  
        }
        else if (direction == MOVE_BACKWARD)
        {
            cameraPosition -= cameraFront * speed;
            cameraTarget -= cameraFront * speed;   
        }
        else if (direction == MOVE_RIGHT)
        {
            cameraPosition += cameraSide * speed;
            cameraTarget += cameraSide * speed;  
        }
        else if (direction == MOVE_LEFT)
        {
            cameraPosition -= cameraSide * speed;
            cameraTarget -= cameraSide * speed;  
        }
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        // Actualizare yaw și pitch
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFrontDirection = glm::normalize(front);

        // Recalculăm direcțiile corelate
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    void Camera::setPosition(glm::vec3 newPosition) {
        this->cameraPosition = newPosition;
    }
    
    glm::vec3 Camera::getPosition() {
        return this->cameraPosition;
    }

    void Camera::setTarget(glm::vec3 newPosition) {
        this->cameraTarget = newPosition;
        
    }

}